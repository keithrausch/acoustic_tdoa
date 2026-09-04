
#include "main_4channel.hpp"


void setup()
{
    // serial setup
    if (verbose)
    {
      Serial.begin(serial_baud);
      Serial.println("starting...");
    }

    // ethernet hardware / udp setup
    if (!Ethernet.begin(mac.begin(), teensy_ip, gateway, dns, subnet))
    {
        Serial.println("Ethernet.begin failed");
    }
    Ethernet.setLocalIP(teensy_ip);
    Ethernet.setSubnetMask(subnet);
    Ethernet.setGatewayIP(gateway);

    while (!Ethernet.linkStatus())
    {
        if (verbose)
        {
          Serial.println("Waiting for link...");
        }
        delay(100);
    }
    udp.begin(udp_port_local);

    // reset our own code
    acquirer.reset(chirp.input);
    acquirer.extremma_helper().reset(n_extremma);

    for (auto & element : last_chirp)
    {
      element.reset();
    }
    acquirer_run_time_us.fill(0);

    //
    // configure audio library
    //
    AudioMemory(1024);

    AudioNoInterrupts(); // turn off interrupts for multi-parameter changes

    auto session_id = trng_random();
    udp_audio_tool.set_session_id(udp_sync, session_id, msg_type_audio_stream);

    // Enable the audio shield, select input, and enable output
    sgtl5000_1.setAddress(LOW);
    sgtl5000_2.setAddress(HIGH);

    sgtl5000_1.enable();
    sgtl5000_2.enable();

    sgtl5000_1.inputSelect(mic_input_select);
    sgtl5000_2.inputSelect(mic_input_select);

    sgtl5000_1.volume(0.9);
    sgtl5000_2.volume(0.9);

    // LETS GO
    for (auto & queue_in : queues_in)
    {
      queue_in.begin();
    }

    AudioInterrupts();
}


void loop()
{
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
        serial_stream << "channel " << c << " - queue size: " << queues_in[c].available();
        serial_stream << ", run time: " << acquirer_run_time_us[c] ;
        serial_stream << "us (EMA)\n";
      }

      Serial.print("block_index ");
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

      TimeIndex tau_to_time_offset_idx = block_index * domain::BlockSize; // tau of 0 corresponds to a peak at this time;

      if (queue_sizes[channel_index] > 0)
      {
        auto src_ptr = queues_in[channel_index].readBuffer();

        auto start_us = micros();
        [[maybe_unused]] auto n_extremma_added = acquirer.run(channel_index, src_ptr, on_chirp, tau_to_time_offset_idx, n_extremma, sync_period_idx, sync_half_gate_idx, nearby_peak_tolerance_idx);
        auto stop_us = micros();
        auto delta_us = stop_us - start_us;

        
        acquirer_run_time_us[channel_index] = run_time_blend_factor * delta_us + (1.0-run_time_blend_factor) * acquirer_run_time_us[channel_index];
        
        queues_in[channel_index].freeBuffer();
        --queue_sizes[channel_index];
      }
    };

    // figure out how many elements we can chew from each input queue
    int Nchew_max = 2;
    int Nchew = Nchew_max; // max chew amount
    for (size_t c = 0; c < Nchannels; ++c)
    {
      Nchew = std::min(Nchew, queue_sizes[c]);
    }

    // process incoming data
    for (int i = 0; i < Nchew; ++i)
    {
      for (size_t c = 0; c < Nchannels; ++c)
      {
        run_on_channel(c);
      }

      ++block_index;
    }

    std::array<int, Nchannels> manual_index_offsets;
    manual_index_offsets.fill(0);

    // now process every combination of microphones
    bool printed_any = false;
    for (size_t i = 0; i < Nchannels-1; ++i)
    {
        auto & chirp_i = last_chirp[i];
        if (0 == chirp_i.time_idx)
        {
          continue;
        }

      TimeIndex time_i_idx = chirp_i.time_idx + manual_index_offsets[i];

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

        TimeIndex time_j_idx = chirp_j.time_idx + manual_index_offsets[j];

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

    // send audio over udp
    if (udp_audio_tool.available())
    {
      auto & buffers = udp_audio_tool.getBuffers();
      for (size_t i = 0; i < Nchannels; ++i)
      {
        auto & buffer = buffers[i];
        auto src_ptr = reinterpret_cast<uint8_t*>(&buffer);
        auto n_bytes = sizeof(buffer);
        udp.send(receiver_ip, udp_port_receiver, src_ptr, n_bytes);
      }

      udp_audio_tool.clearReady();
    }
}