
#include "domain.hpp"
#include "utils.hpp"
#include "specifics.hpp"
#include "serial_stream.hpp"

#include <Wire.h>
#include <SPI.h>
// #include "serial_stream.hpp"

// i need access to the queue size (via header and tail pointers) of Audio/play_queue.h
#define private protected
#include <Audio.h>


constexpr size_t Nchannels = 4;


// GUItool: begin automatically generated code
AudioInputI2SQuad                               i2s_in;
std::array<AudioRecordQueue, Nchannels>         queues_in;
AudioConnection                                 patchCordA(i2s_in, 0, queues_in[0], 0);
AudioConnection                                 patchCordB(i2s_in, 1, queues_in[1], 0);
AudioConnection                                 patchCordC(i2s_in, 2, queues_in[2], 0);
AudioConnection                                 patchCordD(i2s_in, 3, queues_in[3], 0);
AudioControlSGTL5000                            sgtl5000_1;
AudioControlSGTL5000                            sgtl5000_2;
// GUItool: end automatically generated code

// which input on the audio shield will be used?
const int myInput = AUDIO_INPUT_LINEIN; // AUDIO_INPUT_MIC;

uint32_t next_tlm_time_us = 1*1E6;
constexpr uint32_t tlm_period_us = 5.0*1E6;


// chirp
// static constexpr auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
// constexpr utils::WaveParams chirp_params = utils::WaveParams{.amplitude = 30000.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 5E3};
utils::FFTHelper<domain::WindowSize> chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

// implement a search
constexpr size_t n_extremma = 2 * 3 + 1;
constexpr types::Precision sync_period_s = chirp_period_us * 1E-6;
constexpr types::Precision sync_half_gate_s = 10.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5; // TODO MAKE SMALLER
constexpr types::Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;

constexpr size_t sync_period_idx = sync_period_s * domain::cd_freq_hz;
constexpr size_t sync_half_gate_idx = sync_half_gate_s * domain::cd_freq_hz;
constexpr size_t nearby_peak_tolerance_idx = nearby_peak_tolerance_s * domain::cd_freq_hz;

typedef uint32_t IndexT;
typedef float PeakT;
typedef utils::SignalAcquirer<domain::WindowSize, Nchannels, IndexT, PeakT> AcquirerT;
AcquirerT acquirer;
IndexT block_index = 0;

std::array<AcquirerT::ExtremmaFinderT::CandidateExtremma, Nchannels> last_chirp;

constexpr size_t NcachedAudioBuffers = domain::WindowSize / domain::BlockSize;
typedef std::array<int16_t, domain::BlockSize> AudioBuffer;
typedef std::array<AudioBuffer, NcachedAudioBuffers> CachedAudioBuffers; 
CachedAudioBuffers out_waveform_A;
CachedAudioBuffers out_waveform_B;


std::array<double, Nchannels> acquirer_run_time_us;
double blend_factor = 0.01; // use this amount of new information

constexpr bool verbose = true;
ArduinoSerialStream serial_stream{};


void setup()
{
    Serial.begin(115200);
    Serial.println("starting...");

    AudioMemory(1024);

    // Enable the audio shield, select input, and enable output
    sgtl5000_1.setAddress(LOW);
    sgtl5000_2.setAddress(HIGH);

    sgtl5000_1.enable();
    sgtl5000_2.enable();

    sgtl5000_1.inputSelect(myInput);
    sgtl5000_2.inputSelect(myInput);

    sgtl5000_1.volume(0.9);
    sgtl5000_2.volume(0.9);

    acquirer.reset(chirp.input);
    acquirer.extremma_helper().reset(n_extremma);

    // last_chirp.fill(0);
    acquirer_run_time_us.fill(0);

    // LETS GO
    for (auto & queue_in : queues_in)
    {
      queue_in.begin();
    }
}


void loop()
{

//     if (queues_in[0].available() &&
//         queues_in[1].available() &&
//         queues_in[2].available() &&
//         queues_in[3].available())
//     {
// 
//         std::array<uint32_t, 4> sums{};
// 
//         for (size_t c = 0; c < 4; ++c)
//         {
//           sums[c] = 0;
//           int16_t* p = queues_in[c].readBuffer();
//           for (size_t i = 0; i < 128; ++i)
//           {
//             sums[c] += std::abs(static_cast<int>(p[i]));
//           }
//         }
// 
//         Serial.printf("%lu,\t%lu,\t%lu,\t%lu\n",
//             sums[0], sums[1], sums[2], sums[3]);
// 
//         for (auto &q : queues_in)
//             q.freeBuffer();
// 
//         // while (1) {}
//     }

  std::array<int, Nchannels> queue_sizes;
  for (size_t i = 0; i < Nchannels; ++i)
  {
    queue_sizes[i] = queues_in[i].available();
  }

  auto current_time_us = micros(); // TODO convert to uint64_t and handle rollover

  // send tlm
  bool should_tlm = (current_time_us >= next_tlm_time_us);
  for (size_t c = 0; c < Nchannels; ++c)
  {
    should_tlm || queue_sizes[c] > 10;
  }

  if (should_tlm)
  {
    if constexpr(verbose)
    {
      // std::stringstream ss;
      for (size_t c = 0; c < Nchannels; ++c)
      {
        serial_stream << "queue["<<c<<"] - size: " << queues_in[c].available();
        serial_stream << ", run time: " << acquirer_run_time_us[c] ;
        serial_stream << "us (EMA)\n";
      }
      // Serial.print(ss.str().c_str());

      Serial.print("block_index");
      Serial.println(block_index);
    }

    next_tlm_time_us += tlm_period_us;
  }



    std::array<bool, Nchannels> got_chirp;
    got_chirp.fill(false);

    auto on_chirp = [&](size_t channel_index, const AcquirerT::ExtremmaFinderT::CandidateExtremma & det)
    {
        if (channel_index < last_chirp.size())
        {
          got_chirp[channel_index] = true;
          last_chirp[channel_index] = det;
        }
    };

    auto run_on_channel = [&](size_t channel_index)
    {

      IndexT tau_to_time_offset_idx = block_index * domain::BlockSize; // tau of 0 corresponds to a peak at this time;

      if (queue_sizes[channel_index] > 0)
      {
        auto src_ptr = queues_in[channel_index].readBuffer();

        auto start_us = micros();
        auto n_extremma_added = acquirer.run(channel_index, src_ptr, on_chirp, tau_to_time_offset_idx, n_extremma, sync_period_idx, sync_half_gate_idx, nearby_peak_tolerance_idx);
        auto stop_us = micros();
        auto delta_us = stop_us - start_us;

        
        acquirer_run_time_us[channel_index] = blend_factor * delta_us + (1.0-blend_factor) * acquirer_run_time_us[channel_index];
        
        queues_in[channel_index].freeBuffer();
        --queue_sizes[channel_index];

        // optional print
        if constexpr (false)
        {
          serial_stream << "n_extremma_added: " << n_extremma_added << "\n";
          serial_stream << "tau_to_time_offset_idx = " << tau_to_time_offset_idx << "[] block_index = " << block_index << "[]\n";
          // serial_stream << acquirer.extremma_helper().print(serial_stream, domain::sample_period_s);
          // Serial.println(ss.str().c_str());
          // std::cout << "";
        }
      }
    };

    // figure out how many elements we can chew from each input queue
    int Nchew = 2; // max chew amount
    for (size_t c = 0; c < Nchannels; ++c)
    {
      Nchew = std::min(Nchew, queue_sizes[c]);
    }

    // process incoming data
    for (int i = 0; i < Nchew; ++i)
    {

//         int16_t *a = queues_in[0].readBuffer();
//         int16_t *b = queues_in[1].readBuffer();
//         int16_t *c = queues_in[2].readBuffer();
//         int16_t *d = queues_in[3].readBuffer();
// 
//         uint32_t ii = block_index * 128;
//         for (int n = 0; n < 128; ++n)
//         {
//             Serial.printf("%lu,%d,%d,%d,%d\n",
//                 (ii+n), a[n], b[n], c[n], d[n]);
//         }

      for (size_t c = 0; c < Nchannels; ++c)
      {
        run_on_channel(c);
      }

      ++block_index;
    }

    std::array<int, Nchannels> manual_index_offsets{0, 0, 0, 0};

    // now process every combination of microphones
    bool printed_any = false;
    for (size_t i = 0; i < Nchannels-1; ++i)
    {
        auto & chirp_i = last_chirp[i];
        if (0 == chirp_i.time_idx)
        {
          continue;
        }

      IndexT time_i_idx = chirp_i.time_idx + manual_index_offsets[i];

      for (size_t j = i + 1; j < Nchannels; ++j)
      {
        auto & chirp_j = last_chirp[j];

        if (0 == chirp_j.time_idx)
        {
          continue;
        }

        if (!got_chirp[i] && !got_chirp[j])
        {
          continue;
        }

        IndexT time_j_idx = chirp_j.time_idx + manual_index_offsets[j];

        // compute chirp_i.time_idx - chirp_j.time_idx but be careful of unsignd differences
        double delta_ij_idx = 0;
        if (time_i_idx > time_j_idx)
        {
          delta_ij_idx = +1 * static_cast<double>(time_i_idx - time_j_idx);
        }
        else
        {
          delta_ij_idx = -1 * static_cast<double>(time_j_idx - time_i_idx);
        }
        delta_ij_idx += (static_cast<double>(chirp_i.time_idx_fraction) - static_cast<double>(chirp_j.time_idx_fraction));
        double delta_ij_s = delta_ij_idx * domain::sample_period_s;

        auto delta_ij_mm = delta_ij_s * constants::speed_of_sound_mmps;

        // TODO remove me
        if (std::abs(delta_ij_s) > 0.01)
        {
          return;
        }

        if constexpr(verbose)
        {
          // std::stringstream ss;
          serial_stream <<  "chirp detected between channels "<<i<<"&"<<j<<":";
          // serial_stream << std::fixed << std::showpoint << std::showpos;
          // serial_stream << std::setprecision(8);
          serial_stream <<" mic["<<i<<"].time_idx: " << time_i_idx << chirp_i.time_idx_fraction << "[], mic["<<j<<"].time_idx: " << time_j_idx << chirp_j.time_idx_fraction;
          // serial_stream << std::setprecision(2);
          serial_stream << "[], TDOA: " << delta_ij_s*1E6 << "us, linear_intra_mic_distance: " << delta_ij_mm << "mm\n";
          // Serial.print(ss.str().c_str());
          printed_any |= true;
        }
      }
    }
    if (printed_any)
    {
      serial_stream << "\n";
    }
}