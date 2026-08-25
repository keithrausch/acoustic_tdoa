#include "domain.hpp"
#include "utils.hpp"
#include "specifics.hpp"
#include "serial_stream.hpp"

#include <Wire.h>
#include <SPI.h>

// i need access to the queue size (via header and tail pointers) of Audio/play_queue.h
#define private protected
#include <Audio.h>

//
// global config
//
constexpr size_t Nchannels = 4;
constexpr bool verbose = true;
constexpr uint32_t serial_baud = 115200;
uint32_t next_tlm_time_us = 1*1E6;
constexpr uint32_t tlm_period_us = 5.0*1E6;
double run_time_blend_factor = 0.01; // use this amount of new information

auto chirp = utils::FFTHelper<types, domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

ArduinoSerialStream serial_stream{};


//
// acquirer config
//
using AcquirerT =  utils::SignalAcquirer<types, domain::WindowSize, Nchannels>;
AcquirerT acquirer{};

// search params
constexpr size_t n_extremma = 2 * 3 + 1;
constexpr Precision sync_period_s = chirp_period_us * 1E-6;
constexpr Precision sync_half_gate_s = 10.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5; // TODO MAKE SMALLER
constexpr Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;

constexpr size_t sync_period_idx = sync_period_s * domain::cd_freq_hz;
constexpr size_t sync_half_gate_idx = sync_half_gate_s * domain::cd_freq_hz;
constexpr size_t nearby_peak_tolerance_idx = nearby_peak_tolerance_s * domain::cd_freq_hz;


//
// audio library config
//

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

const int mic_input_select = AUDIO_INPUT_LINEIN; // AUDIO_INPUT_MIC;

//
// runtime state
//

TimeIndex block_index = 0;
std::array<AcquirerT::ExtremmaFinderT::CandidateExtremma, Nchannels> last_chirp{};

std::array<double, Nchannels> acquirer_run_time_us;