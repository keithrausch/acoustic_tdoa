#ifndef AUDIOOUTPUTUDP
#define AUDIOOUTPUTUDP

#include <Arduino.h>
#include <AudioStream.h>
#include <array>
#include <Audio.h>

struct UDPAudioBlock
{
    uint64_t sync{0};
    uint16_t msg_type{0};
    uint16_t session_id{0};
    uint16_t channel_id{0};
    uint64_t block_index{0}; // index of first sample in buffer
    std::array<int16_t, AUDIO_BLOCK_SAMPLES> data{};
};

template <size_t Nchannels>
class AudioOutputUDP : public AudioStream
{
public:
    constexpr static size_t block_size{AUDIO_BLOCK_SAMPLES};
    using SampleT = decltype(audio_block_t::data[0]);

    // Create an object with Nchannels input channels
    AudioOutputUDP() : AudioStream(Nchannels, inputQueueArray), packetReady(false)
    {}

    virtual void update()
    {
        audio_block_t *blocks[Nchannels];
        
        // Grab the blocks from the Nchannels input channels
        for (size_t i = 0; i < Nchannels; ++i)
        {
            blocks[i] = receiveReadOnly(i);
        }

        // If the main thread hasn't sent the last packet yet, drop this block to prevent lag
        if (packetReady)
        {
            for (size_t i = 0; i < Nchannels; i++)
            {
                if (blocks[i])
                {
                    release(blocks[i]);
                }
            }

            ++block_count;
            return; 
        }

        // Copy Nchannels channels of block_size samples (16-bit / 2 bytes each) into our staging buffer
        for (size_t ch = 0; ch < Nchannels; ++ch)
        {
            auto &msg = outgoing_messages[ch];
            msg.block_index = block_count;

            auto dst_ptr = msg.data.data();
            size_t n_bytes = block_size * sizeof(SampleT);

            if (blocks[ch])
            {
                auto src_ptr = &(blocks[ch]->data[0]);
                memcpy(dst_ptr, src_ptr, n_bytes);
                release(blocks[ch]);
            }
            else
            {
                // If a channel is empty/disconnected, fill its segment with silence
                memset(dst_ptr, 0, n_bytes);
            }
        }

        ++block_count;

        packetReady = true; // Signal main loop() that data is ready to transmit
    }

    bool available() { return packetReady; }
    std::array<UDPAudioBlock, Nchannels>& getBuffers() { return outgoing_messages; }
    void clearReady() { packetReady = false; }

    void set_session_id(uint64_t sync, uint16_t id, uint16_t msg_type)
    {
        for (size_t i = 0; i < outgoing_messages.size(); ++i)
        {
            auto & msg = outgoing_messages[i];

            msg.sync = sync;
            msg.msg_type = msg_type;
            msg.session_id = id;
            msg.channel_id = i;
        }
    }

private:
    audio_block_t *inputQueueArray[Nchannels];
    std::array<UDPAudioBlock, Nchannels> outgoing_messages{}; // Holds 512 samples total (1024 bytes)
    volatile bool packetReady{false};
    volatile uint64_t block_count{0};
};


#endif