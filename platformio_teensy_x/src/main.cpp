#include "include/utils/domain.hpp"
#include "include/utils/utils.hpp"


int main(int argc, char *argv[])
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 15E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    types::Precision sync_period_s = domain::window_period_s;
    types::Precision sync_half_gate_s = 1.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5;
    types::Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;

    auto chirp_input = chirp.input;

    utils::SignalAcquirer<domain::WindowSize> acquirer;
    acquirer.reset(chirp_input, domain::cd_freq_hz);
    acquirer.extremma_helper().reset(n_extremma);

    auto on_chirp = [&](const utils::ExtremmaFinder::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected every 2 blocks:" << " time_s: " << det.time_s << " value: " << det.value<< " id: " << /*det.id <<*/ "\n";
        std::cout << ss.str();
    };

    size_t block_index = 0;
    while (true)
    {
        auto block_time_s = block_index * domain::block_period_s; // time stamp of the first sample in this block
        auto tau_to_time_offset_s = block_time_s;
        auto signal_ptr = chirp_input.data() + (block_index%2==0 ? 0 : domain::BlockSize);
        acquirer.run(signal_ptr, on_chirp, tau_to_time_offset_s, n_extremma, sync_period_s, sync_half_gate_s, nearby_peak_tolerance_s);
        ++block_index;
    }


    return 0;
}