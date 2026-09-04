#include <Audio.h>
#include <QNEthernet.h>
#include <SPI.h>
#include <Wire.h>

#include "domain.hpp"
#include "specifics.hpp"
#include "serial_stream.hpp"
#include "udp_streamer.hpp"
#include "utils.hpp"
#include "trng.hpp"


using namespace qindesign::network;


//
// global config
//
constexpr size_t Nchannels = 4;
constexpr bool verbose = false;
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
AudioConnection                                 patchCord_A(i2s_in, 0, queues_in[0], 0);
AudioConnection                                 patchCord_B(i2s_in, 1, queues_in[1], 0);
AudioConnection                                 patchCord_C(i2s_in, 2, queues_in[2], 0);
AudioConnection                                 patchCord_D(i2s_in, 3, queues_in[3], 0);
AudioControlSGTL5000                            sgtl5000_1;
AudioControlSGTL5000                            sgtl5000_2;
// GUItool: end automatically generated code

AudioOutputUDP<Nchannels> udp_audio_tool{};
AudioConnection                                 patchCordUDP_A(i2s_in, 0, udp_audio_tool, 0);
AudioConnection                                 patchCordUDP_B(i2s_in, 1, udp_audio_tool, 1);
AudioConnection                                 patchCordUDP_C(i2s_in, 2, udp_audio_tool, 2);
AudioConnection                                 patchCordUDP_D(i2s_in, 3, udp_audio_tool, 3);

const int mic_input_select = AUDIO_INPUT_LINEIN; // AUDIO_INPUT_MIC;

//
// ethernet library config
//

std::array<uint8_t, 6> mac{ 0x04, 0xE9, 0xE5, 0x00, 0x00, 0x01};

IPAddress receiver_ip(192, 168, 1, 100);
IPAddress teensy_ip(192, 168, 1, 50);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 1, 1);
IPAddress dns(gateway);

constexpr uint16_t udp_port_local = 4000;
constexpr uint16_t udp_port_receiver  = 5000;

EthernetUDP udp;
uint64_t udp_sync = 4072785483;
uint16_t msg_type_audio_stream = 1;
uint16_t msg_type_other = 2;


//
// runtime state
//

TimeIndex block_index = 0;
std::array<AcquirerT::ExtremmaFinderT::CandidateExtremma, Nchannels> last_chirp{};

std::array<double, Nchannels> acquirer_run_time_us;