
#include "domain.hpp"
#include "utils.hpp"

#include <SparkFun_WM8960_Arduino_Library.h> 

#include <Wire.h>
#include <SPI.h>
#include <SerialFlash.h>

// i need access to the queue size (via header and tail pointers) of Audio/play_queue.h
#define private protected
#include <Audio.h>

class RauschAudioPlayQueue : public AudioPlayQueue
{
  void update(void)
  {
    uint32_t t = tail;
    if (t != head) {
      ++count_updates_with_data;
    }
    else
    {
      ++count_updates_without_data;
    }

    AudioPlayQueue::update();
  }

  public:
  volatile uint32_t count_updates_with_data{0};
  volatile uint32_t count_updates_without_data{0};
};



constexpr size_t Nchannels = 2;


// GUItool: begin automatically generated code
AudioInputI2S                                   i2s_in;             //xy=105,63
std::array<AudioRecordQueue, Nchannels>         queues_in;         //xy=281,63
// AudioRecordQueue                                queue_in_A;         //xy=281,63
// AudioRecordQueue                                queue_in_B;         //xy=281,63
AudioConnection                                 patchCord1(i2s_in, 0, queues_in[0], 0);
AudioConnection                                 patchCord2(i2s_in, 1, queues_in[1], 0);

RauschAudioPlayQueue /* was AudioPlayQueue */   queue_out_A;         //xy=520,322
RauschAudioPlayQueue /* was AudioPlayQueue */   queue_out_B;         //xy=562,229
AudioOutputI2S                                  i2s_out;             //xy=735,255
AudioConnection                                 patchCord3(queue_out_A, 0, i2s_out, 1);
AudioConnection                                 patchCord4(queue_out_B, 0, i2s_out, 0);
AudioControlSGTL5000                            sgtl5000_1;     //xy=265,212
// GUItool: end automatically generated code

// which input on the audio shield will be used?
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

constexpr size_t n_blocks_for_chirp_cycle = 344/2;

// uint32_t next_chirp_time_us = 2*1E6;
constexpr uint32_t chirp_period_us = n_blocks_for_chirp_cycle*domain::block_period_s*1E6;

uint32_t next_tlm_time_us = 1*1E6;
constexpr uint32_t tlm_period_us = 5.0*1E6;


// chirp
static constexpr auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
constexpr utils::WaveParams chirp_params = utils::WaveParams{.amplitude = 30000.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 5E3};
utils::FFTHelper<domain::WindowSize> chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

// implement a search
constexpr size_t n_extremma = 2 * 3 + 1;
constexpr types::Precision sync_period_s = chirp_period_us * 1E-6;
constexpr types::Precision sync_half_gate_s = 10.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5; // TODO MAKE SMALLER
constexpr types::Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;
utils::SignalAcquirer<domain::WindowSize, Nchannels> acquirer;
size_t block_index = 0;

std::array<utils::ExtremmaFinder::CandidateExtremma, Nchannels> last_chirp;

constexpr size_t NcachedAudioBuffers = domain::WindowSize / domain::BlockSize;
typedef std::array<std::array<int16_t, domain::BlockSize>, NcachedAudioBuffers> CachedAudioBuffers; 
CachedAudioBuffers out_waveform_A;
CachedAudioBuffers out_waveform_B;


std::array<double, Nchannels> acquirer_run_time_us;
double blend_factor = 0.01; // use this amount of new information


void setup_play_queue(AudioPlayQueue &queue)
{
  // queue.setBehaviour(AudioPlayQueue::NON_STALLING);
  queue.setMaxBuffers(2); // documentation says minimum number of buffers is 2
}

void enqueue_and_play_waveform(AudioPlayQueue &queue, const CachedAudioBuffers& src)
{
  static_assert(NcachedAudioBuffers >= 2, "must queue up at least 2 buffers at a time?");

  for (const auto & buf : src)
  {
    int16_t* dst = queue.getBuffer();

    if (! dst)
    {
      return;
    }

    memcpy(dst, buf.data(), buf.size() * sizeof(*dst));
    queue.playBuffer(); // this adds this to the output queue. will begin playing next possible chance
  }
}


void setup()
{
    Serial.begin(115200);
    Serial.println("starting...");

    AudioMemory(1024);

    // Enable the audio shield, select input, and enable output
    sgtl5000_1.enable();
    sgtl5000_1.inputSelect(myInput);
    sgtl5000_1.volume(0.9);

    // inputs
    // queue1.begin();
    // queue2.begin();

    auto copy_chirp_to_dst = [&](CachedAudioBuffers & output_buffers, auto &chirp, double digital_amplitude)
    {
      for (size_t src = 0; src < chirp.input.size(); ++src)
      {
        size_t buffer_index = src / BUFFER_LENGTH;
        size_t dst = src % BUFFER_LENGTH;
        output_buffers[buffer_index][dst] = digital_amplitude * chirp.input[src];
      }
    };

    double digital_volume_A = 1.0;
    double digital_volume_B = 0.0;
    copy_chirp_to_dst(out_waveform_A, chirp, digital_volume_A);
    copy_chirp_to_dst(out_waveform_B, chirp, digital_volume_B);


    setup_play_queue(queue_out_A);
    setup_play_queue(queue_out_B);

    acquirer.reset(chirp.input, domain::cd_freq_hz);
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
  std::array<int, Nchannels> queue_sizes;
  for (size_t i = 0; i < Nchannels; ++i)
  {
    queue_sizes[i] = queues_in[i].available();
  }
  // if (queue_in_A_size != queue_in_B_size)
  // {
  //     Serial.println("!=");
  //     return;
  // }

  auto current_time_us = micros();

  // play chirp
  // bool should_chirp = current_time_us >= next_chirp_time_us;
  bool should_chirp = ((queue_out_A.count_updates_without_data + queue_out_A.count_updates_with_data) % n_blocks_for_chirp_cycle) == 0;
  if (should_chirp)
  {
    enqueue_and_play_waveform(queue_out_A, out_waveform_A);
    enqueue_and_play_waveform(queue_out_B, out_waveform_B);

    // next_chirp_time_us += chirp_period_us;
  }

  // send tlm
  bool should_tlm = (current_time_us >= next_tlm_time_us);
  for (size_t c = 0; c < Nchannels; ++c)
  {
    should_tlm || queue_sizes[c] > 10;
  }

  if (should_tlm)
  {
    std::stringstream ss;
    for (size_t c = 0; c < Nchannels; ++c)
    {
      ss << "queue["<<c<<"] - size: " << queues_in[c].available();
      ss << ", run time: " << acquirer_run_time_us[c] ;
      ss << "us (EMA)\n";
    }
    Serial.print(ss.str().c_str());

    Serial.print("block_index");
    Serial.println(block_index);

    next_tlm_time_us += tlm_period_us;
  }



    std::array<bool, Nchannels> got_chirp;
    got_chirp.fill(false);

    auto on_chirp = [&](size_t channel_index, const utils::ExtremmaFinder::CandidateExtremma & det)
    {
        if (channel_index < last_chirp.size())
        {
          got_chirp[channel_index] = true;
          last_chirp[channel_index] = det;
        }
    };

    auto run_on_channel = [&](size_t c /* channel index*/)
    {

      auto block_time_s = block_index * domain::block_period_s; // time stamp of the first sample in this block
      auto tau_to_time_offset_s = block_time_s;

      if (queue_sizes[c] > 0)
      {
        auto src_ptr = queues_in[c].readBuffer();

        auto start_us = micros();
        auto n_extremma_added = acquirer.run(c, src_ptr, on_chirp, tau_to_time_offset_s, n_extremma, sync_period_s, sync_half_gate_s, nearby_peak_tolerance_s);
        auto stop_us = micros();
        auto delta_us = stop_us - start_us;

        
        acquirer_run_time_us[c] = blend_factor * delta_us + (1.0-blend_factor) * acquirer_run_time_us[c];
        
        queues_in[c].freeBuffer();
        --queue_sizes[c];

        // optional print
        // if (false)
        // {
        //   std::stringstream ss;
        //   ss << "n_extremma_added: " << n_extremma_added << "\n";
        //   ss << "block_time_s = " << block_time_s << "s block_index = " << block_index << "[]\n";
        //   ss << acquirer.extremma_helper().print(domain::sample_period_s);
        //   Serial.println(ss.str().c_str());
        //   std::cout << "";
        // }
      }
    };

    int Nchew = 2; // max chew amount
    for (size_t c = 0; c < Nchannels; ++c)
    {
      Nchew = std::min(Nchew, queue_sizes[c]);
    }

    for (int i = 0; i < Nchew; ++i)
    {
      for (size_t c = 0; c < Nchannels; ++c)
      {
        run_on_channel(c);
      }

      ++block_index;
    }

    // new process every combination of microphones
    for (size_t i = 0; i < Nchannels-1; ++i)
    {
        auto & chirp_i = last_chirp[i];
        if (std::isnan(chirp_i.time_s))
        {
          continue;
        }

      for (size_t j = i + 1; j < Nchannels; ++j)
      {
        auto & chirp_j = last_chirp[j];

        if (std::isnan(chirp_j.time_s))
        {
          continue;
        }

        if (!got_chirp[i] && !got_chirp[j])
        {
          continue;
        }

        auto delta_s = chirp_i.time_s - chirp_j.time_s;
        auto delta_mm = delta_s * constants::speed_of_sound_mmps;

        if (std::fabs(delta_s) > 0.01)
        {
          return;
        }

        std::stringstream ss;
        ss <<  "chirp detected between channels "<<i<<"&"<<j<<":";
        ss << std::fixed << std::showpoint << std::showpos;
        ss << std::setprecision(8);
        ss <<" mic["<<i<<"].time: " << chirp_i.time_s << "s, mic["<<j<<"].time: " << chirp_j.time_s;
        ss << std::setprecision(2);
        ss << "s, TDOA: " << delta_s*1E6 << "us, linear_intra_mic_distance: " << delta_mm << "mm\n";

        Serial.print(ss.str().c_str());
      }
    }
}