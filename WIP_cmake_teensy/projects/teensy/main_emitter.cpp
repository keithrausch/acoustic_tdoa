
#include "domain.hpp"
#include "utils.hpp"
#include "specifics.hpp"

#include <Wire.h>
#include <SPI.h>

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





// GUItool: begin automatically generated code
RauschAudioPlayQueue /* was AudioPlayQueue */   queue_out_A;
RauschAudioPlayQueue /* was AudioPlayQueue */   queue_out_B;
RauschAudioPlayQueue /* was AudioPlayQueue */   queue_out_C;
RauschAudioPlayQueue /* was AudioPlayQueue */   queue_out_D;
AudioOutputI2SQuad                                  i2s_out;
AudioConnection                                 patchCordA(queue_out_A, 0, i2s_out, 0);
AudioConnection                                 patchCordB(queue_out_B, 0, i2s_out, 1);
AudioConnection                                 patchCordC(queue_out_C, 0, i2s_out, 2);
AudioConnection                                 patchCordD(queue_out_D, 0, i2s_out, 3);
AudioControlSGTL5000                            sgtl5000_1;
AudioControlSGTL5000                            sgtl5000_2;
// GUItool: end automatically generated code

// constexpr size_t n_blocks_for_chirp_cycle = 344/2;
// 
// constexpr uint32_t chirp_period_us = n_blocks_for_chirp_cycle*domain::block_period_s*1E6;
// 
// 
// // chirp
// static constexpr auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
// constexpr utils::WaveParams chirp_params = utils::WaveParams{.amplitude = 30000.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 5E3};
utils::FFTHelper<domain::WindowSize> chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);


constexpr size_t NcachedAudioBuffers = domain::WindowSize / domain::BlockSize;
typedef std::array<int16_t, domain::BlockSize> AudioBuffer;
typedef std::array<AudioBuffer, NcachedAudioBuffers> CachedAudioBuffers; 
CachedAudioBuffers out_waveform_A;
CachedAudioBuffers out_waveform_B;
CachedAudioBuffers out_waveform_C;
CachedAudioBuffers out_waveform_D;


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
    sgtl5000_1.setAddress(LOW);
    sgtl5000_1.enable();
    sgtl5000_1.volume(0.9);

    sgtl5000_2.setAddress(HIGH);
    sgtl5000_2.enable();
    sgtl5000_2.volume(0.9);

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
    double digital_volume_B = 1.0*0;
    double digital_volume_C = 1.0*0;
    double digital_volume_D = 1.0*0;
    copy_chirp_to_dst(out_waveform_A, chirp, digital_volume_A);
    copy_chirp_to_dst(out_waveform_B, chirp, digital_volume_B);
    copy_chirp_to_dst(out_waveform_C, chirp, digital_volume_C);
    copy_chirp_to_dst(out_waveform_D, chirp, digital_volume_D);


    setup_play_queue(queue_out_A);
    setup_play_queue(queue_out_B);
    setup_play_queue(queue_out_C);
    setup_play_queue(queue_out_D);
}


void loop()
{
  // auto current_time_us = micros(); // TODO convert to uint64_t and handle rollover

  // play chirp
  bool should_chirp = ((queue_out_A.count_updates_without_data + queue_out_A.count_updates_with_data) % n_blocks_for_chirp_cycle) == 0;
  if (should_chirp)
  {
    enqueue_and_play_waveform(queue_out_A, out_waveform_A);
    enqueue_and_play_waveform(queue_out_B, out_waveform_B);
    enqueue_and_play_waveform(queue_out_C, out_waveform_C);
    enqueue_and_play_waveform(queue_out_D, out_waveform_D);
  }
}