// Record sound as raw data to a SD card, and play it back.
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// Three pushbuttons need to be connected:
//   Record Button: pin 0 to GND
//   Stop Button:   pin 1 to GND
//   Play Button:   pin 2 to GND
//
// This example code is in the public domain.

#include <SparkFun_WM8960_Arduino_Library.h> 

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioInputI2S            i2s_in;             //xy=105,63
AudioRecordQueue         queue_in_A;         //xy=281,63
AudioRecordQueue         queue_in_B;         //xy=281,63
AudioConnection          patchCord1(i2s_in, 0, queue_in_A, 0);
AudioConnection          patchCord2(i2s_in, 1, queue_in_B, 0);

AudioPlayQueue           queue_out_A;         //xy=520,322
AudioPlayQueue           queue_out_B;         //xy=562,229
AudioOutputI2S           i2s_out;             //xy=735,255
AudioConnection          patchCord3(queue_out_A, 0, i2s_out, 1);
AudioConnection          patchCord4(queue_out_B, 0, i2s_out, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=265,212
// GUItool: end automatically generated code

static const int BUF_LENGTH = 128;
static const int HALF_BUF_LENGTH = BUF_LENGTH/2;
int16_t out_waveform_A[BUF_LENGTH];
int16_t out_waveform_B[BUF_LENGTH];

// which input on the audio shield will be used?
const int myInput = AUDIO_INPUT_LINEIN;
//const int myInput = AUDIO_INPUT_MIC;

bool recording = false;
uint32_t stop_time = 0;


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

    // outputs
    double amplitude = 5000;
    double w = AUDIO_SAMPLE_RATE_EXACT / BUF_LENGTH;
    double a1 = 0;
    double b1 = 5*w;
    double a2 = b1+w*5;
    double b2 = a2+w*5;
    build_morlet_freq_control(out_waveform_A, amplitude, a1, b1, +1);
    build_morlet_freq_control(out_waveform_B, 0*amplitude, a2, b2, +1);
    setup_play_queue(queue_out_A);
    setup_play_queue(queue_out_B);
}


void loop()
{
  // auto current_time = micros();

  if (Serial.available() > 0)
  {
    while (Serial.available() > 0)
    {
      Serial.read();
    }
    // uint32_t duration = 0.002*1E6;
    // stop_time = current_time + duration;
    start_recording();
  }

  if (queue_in_A.available() > 3)
  {
    stop_recording();
  // }

  // if (current_time >= stop_time && recording)
  // {
  //   stop_recording();

    print_header();
    Serial.print('A');
    print_recording(queue_in_A);

    print_header();
    Serial.print('B');
    print_recording(queue_in_B);
  }

}


// void process_recording(AudioRecordQueue &queue, bool verbose) {

//   int count = queue.available();

//   if (verbose)
//   {
//     Serial.print("count:" + String(count) + "\n");
//   }

//   while (count--)
//   {
//     // memcpy(buffer, queue.readBuffer(), 256);
//     queue.freeBuffer();
//   }
// }
void print_recording(AudioRecordQueue &queue) {

  int count = queue.available();

  // Serial.print(count);


  while (count--)
  {
    // memcpy(buffer, queue.readBuffer(), 256);
    Serial.write((uint8_t*)queue.readBuffer(), sizeof(queue.readBuffer()[0])*BUF_LENGTH);
    Serial.flush();
    queue.freeBuffer();
  }
}


void print_header()
{
  for (int i = 0; i < 8; ++i)
  {
    Serial.print('x');
  }
}

void start_recording()
{
  ready_play_queue(queue_out_A, out_waveform_A);
  ready_play_queue(queue_out_B, out_waveform_B);

  queue_out_A.playBuffer();
  queue_out_B.playBuffer();

  recording = true;
  queue_in_A.begin();
  queue_in_B.begin();


}

void stop_recording()
{
  queue_in_A.end();
  queue_in_B.end();
  recording = false;
}

// void build_morlet_freq(int16_t* dst, double amp, double freq, double width)
// {
//   if (! dst)
//   {
//     return;
//   }

//   for (int i = 0; i < BUF_LENGTH; ++i)
//   {
//     double t = (i-HALF_BUF_LENGTH) * 1.0/(double)AUDIO_SAMPLE_RATE_EXACT; // convert index to time and center it in the window
//     double val = amp * cos( TWO_PI*freq*t) * exp(-(t/width)*(t/width));
//     dst[i] = val;
//   }
// }

void build_morlet_freq_control(int16_t* dst, double amp, double a, double b, double sign)
{
  if (! dst)
  {
    return;
  }

  for (int i = 0; i < BUF_LENGTH; ++i)
  {
    double time = (i-HALF_BUF_LENGTH) * 1.0/(double)AUDIO_SAMPLE_RATE_EXACT; // convert index to time and center it in the window
    double arg = PI*time*(b-a);
    double sinc = sin(arg)/ arg;
    double val = amp * cos(PI*time*(b+a)) * sinc * sinc;

    val = (i==HALF_BUF_LENGTH) ? amp : val;
    dst[i] = val;
  }

  // for (int i = 0; i < BUF_LENGTH; ++i)
  // {
  //   dst[i] = 0;
  // }
  // dst[HALF_BUF_LENGTH] = amp;
  // dst[HALF_BUF_LENGTH] = 0;
  // dst[HALF_BUF_LENGTH+2] = amp*sign;
}

void setup_play_queue(AudioPlayQueue &queue)
{
  // queue.setBehaviour(AudioPlayQueue::NON_STALLING);
  queue.setMaxBuffers(2); // documentation says minimum number of buffers is 2
}

void ready_play_queue(AudioPlayQueue &queue, int16_t* src)
{
  int16_t* dst = queue.getBuffer();

  if (! src || ! dst)
  {
    return;
  }

  memcpy(dst, src, BUF_LENGTH * sizeof(*dst));
}
