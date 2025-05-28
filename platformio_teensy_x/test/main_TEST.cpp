
#include <gtest/gtest.h>

#include "include/utils/domain.hpp"
#include "include/utils/utils.hpp"

constexpr types::Precision residual_s_tolerance = 0.5 * domain::sample_period_s;
constexpr types::Precision residual_mm_tolerance_tight = 1.0;
constexpr types::Precision residual_mm_tolerance_loose = 2.5;

static utils::WaveParams get_offset_params(types::Precision sample_period_s, const utils::WaveParams &chirp_params, types::Precision offset_s)
{
    types::Precision sinc_insertion_time_s = chirp_params.center_s + offset_s;
    return utils::WaveParams{.amplitude = chirp_params.amplitude, .center_s = sinc_insertion_time_s, .freq_hz = chirp_params.freq_hz};
}

TEST(ReconstructionTest, ReconstructionTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    double largest_error = std::numeric_limits<double>::lowest();
    for (size_t k = 0; k < domain::WindowSize; ++k)
    {
        auto reconstructed = utils::reconstruct_at_index(chirp.coeffs, k, chirp.n_lowest);
        double this_error = std::abs(reconstructed - chirp.input[k]);
        largest_error = std::max(largest_error, this_error);
    }
    // std::cout << "largest reconstruction error: " << (largest_error) << "\n";

    ASSERT_LT(largest_error, 1E-4);
}


TEST(DerivativeHelperTest, DerivativeHelperTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::DerivativeHelper<domain::WindowSize, 2> derivative_helper;
    derivative_helper.fill_frequencies(domain::cd_freq_hz);

    auto correlate_and_derive = [&]<size_t derivative_order>(auto tau)
    {
        constexpr bool real_only = true;
        constexpr bool coeffA_already_conjugated = true;
        return derivative_helper.correlate_and_derive<derivative_order, real_only, coeffA_already_conjugated>(chirp.coeffs, signal.coeffs, domain::window_period_s, tau);
    };

    auto fd0_fd1 = [&](auto tau)
    { return correlate_and_derive.template operator()<1>(tau); };
    auto fd0_fd1_fd2 = [&](auto tau)
    { return correlate_and_derive.template operator()<2>(tau); };

    auto eval_and_print = [&](auto tau)
    {
        derivative_helper.eval_and_print(chirp.coeffs, signal.coeffs, domain::window_period_s, tau);
    };

    auto guessed_tau_s = 0 + 0.5 * domain::sample_period_s;
    std::cout << "starting: ";
    eval_and_print(guessed_tau_s);
    std::cout << "solution: ";
    auto optimal_tau_s = std::get<0>(utils::newton(fd0_fd1_fd2, guessed_tau_s));
    eval_and_print(optimal_tau_s);
    std::cout << "tau_true: ";
    eval_and_print(tau_true_s);

    auto residual_s = (tau_true_s - optimal_tau_s);
    auto residual_mm = constants::speed_of_sound_mmps * residual_s;
    std::cout << "residual: " << residual_s << "s, " << residual_mm << "mm\n";

    ASSERT_LT(std::fabs(residual_s), residual_s_tolerance);
    ASSERT_LT(std::fabs(residual_mm), residual_mm_tolerance_tight);
}


TEST(ExtremmaFinderTest, ExtremmaFinderTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::DerivativeHelper<domain::WindowSize, 2> derivative_helper;
    derivative_helper.fill_frequencies(domain::cd_freq_hz);

    auto correlate_and_derive = [&]<size_t derivative_order>(auto tau)
    {
        constexpr bool real_only = true;
        constexpr bool coeffA_already_conjugated = true;
        return derivative_helper.correlate_and_derive<derivative_order, real_only, coeffA_already_conjugated>(chirp.coeffs, signal.coeffs, domain::window_period_s, tau);
    };

    auto fd0 = [&](auto tau)
    { return correlate_and_derive.template operator()<0>(tau)[0]; };
    auto fd1 = [&](auto tau)
    { return correlate_and_derive.template operator()<1>(tau)[1]; };
    auto fd0_fd1_fd2 = [&](auto tau)
    { return correlate_and_derive.template operator()<2>(tau); };

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    auto tau_s_lower_bound = -1 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
    auto tau_s_upper_bound = +1 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
    auto tau_s_step = domain::sample_period_s;
    utils::ExtremmaFinder extremma_helper;
    extremma_helper.reset(n_extremma);
    extremma_helper.find_extremma(fd0, fd1, fd0_fd1_fd2, tau_s_lower_bound, tau_s_upper_bound, tau_s_step);

    // std::cout << "max heap:\n";
    // extremma_helper.print(domain::sample_period_s);

    auto residual_s = extremma_helper.extremma()[0].time_s - tau_true_s;
    auto residual_mm = residual_s * constants::speed_of_sound_mmps;

    ASSERT_LT(std::fabs(residual_s), residual_s_tolerance);
    ASSERT_LT(std::fabs(residual_mm), residual_mm_tolerance_tight);
}

TEST(IngestorTest, IngestorSimpleTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;
    tau_true_s += domain::block_period_s; // the peak is in the second block. this time is now in the absolute frame, not relative to the first block
    // types::Precision sinc_insertion_time_s = signal_params.center_s;

    size_t n_extremma = 2 * 3 + 1;

    std::cout << "run ingestor on sinc-sinc data\n";
    {
        auto chirp_input = chirp.input;

        utils::Ingestor<domain::WindowSize> ingestor;
        ingestor.reset(chirp_input, domain::cd_freq_hz);
        constexpr bool reset_heap = true;
        types::Precision tau_to_time_offset_s{};
        size_t block_index{};
        auto signal_ptr = signal.input.data();
        // std::cout << "sinc_insertion_time_s: " << sinc_insertion_time_s << "\n";

        tau_to_time_offset_s = block_index*domain::block_period_s;
        signal_ptr = signal.input.data() + block_index * domain::BlockSize;
        std::cout << "found peaks in first half (dirty start):\n";
        std::cout << "added " << ingestor.run(signal_ptr, tau_to_time_offset_s, n_extremma, reset_heap) << " extremma\n";
        ingestor.extremma_helper().print(domain::sample_period_s);
        ++block_index;

        tau_to_time_offset_s = block_index*domain::block_period_s;
        signal_ptr = signal.input.data() + block_index * domain::BlockSize;
        std::cout << "found peaks whole window (now warmed up):\n";
        std::cout << "added " << ingestor.run(signal_ptr, tau_to_time_offset_s, n_extremma, reset_heap) << " extremma\n";
        ingestor.extremma_helper().print(domain::sample_period_s, tau_to_time_offset_s);
        ++block_index;


        auto residual_s = ingestor.extremma_helper().extremma()[0].time_s - tau_true_s;
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;

        ASSERT_LT(std::fabs(residual_s), residual_s_tolerance);
        ASSERT_LT(std::fabs(residual_mm), residual_mm_tolerance_tight);
    }
}

template <bool ChirpsOnly=false>
static auto generate_random_sound_for_domain(const types::SoundFunctionT &chirp_func, const utils::WaveParams &chirp_params, size_t n_chirps, types::Precision sync_period_s=1.0)
{
    std::vector<utils::WaveParams> sound_params;
    auto sound_amplitude = 0.5 * chirp_params.amplitude;
    if (!ChirpsOnly)
    {
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.0 * domain::window_period_s, .freq_hz = 2*5.0 * chirp_params.freq_hz});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.3 * domain::window_period_s, .freq_hz = 2*4.0 * chirp_params.freq_hz});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.4 * domain::window_period_s, .freq_hz = 2*3.3 * chirp_params.freq_hz});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.5 * domain::window_period_s, .freq_hz = 2*2.3 * chirp_params.freq_hz});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.6 * domain::window_period_s, .freq_hz = 2*1.3 * chirp_params.freq_hz});
    }
    std::vector<std::pair<utils::WaveParams, types::SoundFunctionT>> multi_chirp_params;
    for (size_t i = 0; i < n_chirps; ++i)
    {
        auto chirp_center_s = i*sync_period_s + 0.5;
        multi_chirp_params.push_back(std::make_pair(utils::WaveParams{.amplitude = chirp_params.amplitude, .center_s = chirp_center_s, .freq_hz = chirp_params.freq_hz}, chirp_func));
    }
    auto generate_sound = [=](types::Precision t)
    { return utils::generic_sound(t, sound_params, multi_chirp_params); };
    const auto duration_s = (n_chirps+0.5) * sync_period_s;
    const size_t n_samples = (duration_s * domain::cd_freq_hz) + domain::BlockSize;
    std::shared_ptr<size_t> n_generated = std::make_shared<size_t>(0);
    auto get_next_block = [generate_sound, n_generated /* stateful */]()
    { 
        auto & count = *n_generated;
        auto signal_block = utils::create_template<domain::BlockSize>(count, domain::sample_period_s, generate_sound);
        count += domain::BlockSize;
        return signal_block;
    };
    // auto signal_inputs = utils::create_template(n_samples, domain::sample_period_s, generate_sound);
    return std::make_tuple(n_samples, get_next_block, multi_chirp_params);
}

TEST(IngestorTest, IngestorLongTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;

    std::cout << "\n\n";
    std::cout << "run ingestor on long running, generic audio\n";

    constexpr size_t n_chirps = 3;
    constexpr bool chirps_only = true;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps);

    auto chirp_input = chirp.input;

    utils::Ingestor<domain::WindowSize> ingestor;
    ingestor.reset(chirp_input, domain::cd_freq_hz);
    ingestor.extremma_helper().reset(n_extremma);

    auto n_blocks = n_samples / domain::BlockSize; // signal_inputs.size() / domain::BlockSize;
    for (size_t block_index = 0; block_index < n_blocks; ++block_index)
    {
        size_t sample_index = block_index * domain::BlockSize;
        auto block_time_s = block_index * domain::block_period_s;
        constexpr bool reset_heap = false;
        auto tau_to_time_offset_s = block_time_s;
        auto signal_block = get_next_block();
        auto signal_ptr = signal_block.data();// signal_inputs.data() + block_index * domain::BlockSize;
        auto n_extremma_added = ingestor.run(signal_ptr, tau_to_time_offset_s, n_extremma, reset_heap);

        // optional print
        if (0 == block_index || block_index == n_blocks-1)
        {
            std::cout << "block_time_s = " << block_time_s << "s block_index = " << block_index << "[], sample_index = " << sample_index << "[]\n";
            ingestor.extremma_helper().print(domain::sample_period_s);
            std::cout << "";
        }
    }

    std::cout << "residuals for the N largest peaks\n";
    utils::Ingestor<domain::WindowSize>::vector_extremma extremma_sorted = ingestor.extremma_helper().extremma(); // deep copy
    extremma_sorted.resize(n_chirps);
    std::sort(extremma_sorted.begin(), extremma_sorted.end(), [](const utils::ExtremmaFinder::CandidateExtremma & ex1, const utils::ExtremmaFinder::CandidateExtremma & ex2){return ex1.time_s < ex2.time_s;});
    for (size_t i = 0; i < n_chirps; ++i)
    {
        auto residual_s = multi_chirp_params[i].first.center_s - extremma_sorted[i].time_s;
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;
        auto residual_percentsamplerate = residual_s / domain::sample_period_s * 100.0;
        std::cout << "chirp["<<i<<"]: residual_s: " << residual_s << "s, residual_mm: " << residual_mm << "mm, residual_%samplerate: " <<residual_percentsamplerate<< "%\n";

        EXPECT_LT(std::fabs(residual_s), residual_s_tolerance);
        EXPECT_LT(std::fabs(residual_mm), residual_mm_tolerance_tight);
    }
    
}

TEST(SignalLockTest, SignalLockTest_LF_NoEnvSound)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    types::Precision sync_period_s = 1.0;
    types::Precision sync_half_gate_s = 1.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5;
    types::Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;

    constexpr size_t n_chirps = 10;
    constexpr bool chirps_only = true;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps, sync_period_s);

    auto chirp_input = chirp.input;

    utils::SignalAcquirer<domain::WindowSize> acquirer;
    acquirer.reset(chirp_input, domain::cd_freq_hz);
    acquirer.extremma_helper().reset(n_extremma);

    std::vector<utils::ExtremmaFinder::CandidateExtremma> detected;
    auto on_chirp = [&](const utils::ExtremmaFinder::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " time_s: " << det.time_s << " value: " << det.value<< /*" id: " << det.id << */"\n";
        std::cout << ss.str();
        detected.push_back(det);
    };

    auto n_blocks = n_samples / domain::BlockSize;;// signal_inputs.size() / domain::BlockSize;
    for (size_t block_index = 0; block_index < n_blocks; ++block_index)
    {
        size_t sample_index = block_index * domain::BlockSize;
        auto block_time_s = block_index * domain::block_period_s; // time stamp of the first sample in this block
        auto tau_to_time_offset_s = block_time_s;
        auto signal_block = get_next_block();
        auto signal_ptr = signal_block.data(); // signal_inputs.data() + block_index * domain::BlockSize;
        acquirer.run(signal_ptr, on_chirp, tau_to_time_offset_s, n_extremma, sync_period_s, sync_half_gate_s, nearby_peak_tolerance_s);


        // optional print
        if (/*0 == block_index ||*/ block_index == n_blocks-1)
        {
            std::cout << "block_time_s = " << block_time_s << "s block_index = " << block_index << "[], sample_index = " << sample_index << "[]\n";
            acquirer.extremma_helper().print(domain::sample_period_s);
            std::cout << "";
        }
    }

    auto n_detected_chirps = detected.size();
    auto n_expected_chirps = n_chirps - acquirer.nDetsForHypothesis + 1;
    ASSERT_EQ(n_expected_chirps, n_detected_chirps) << "didnt detect the right number of chirps";

    auto offset = acquirer.nDetsForHypothesis - 1; // because the first N chirps are missed
    for (size_t i = 0; i < n_chirps-offset; ++i)
    {
        auto & params_this_chirp = multi_chirp_params[i+offset];
        auto residual_s = params_this_chirp.first.center_s - detected[i].time_s;
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;
        auto residual_percentsamplerate = residual_s / domain::sample_period_s * 100.0;
        std::cout << "chirp["<<i<<"]: residual_s: " << residual_s << "s, residual_mm: " << residual_mm << "mm, residual_%samplerate: " <<residual_percentsamplerate<< "%\n";

        EXPECT_LT(std::fabs(residual_s), residual_s_tolerance);
        EXPECT_LT(std::fabs(residual_mm), residual_mm_tolerance_tight);
    }

}


TEST(SignalLockTest, SignalLockTest_HF)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 15E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params, true);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    types::Precision sync_period_s = 0.5;
    types::Precision sync_half_gate_s = 1.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5;
    types::Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;

    constexpr size_t n_chirps = 300;
    constexpr bool chirps_only = false;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps, sync_period_s);

    auto chirp_input = chirp.input;

    utils::SignalAcquirer<domain::WindowSize> acquirer;
    acquirer.reset(chirp_input, domain::cd_freq_hz);
    acquirer.extremma_helper().reset(n_extremma);

    std::vector<utils::ExtremmaFinder::CandidateExtremma> detected;
    auto on_chirp = [&](const utils::ExtremmaFinder::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " time_s: " << det.time_s << " value: " << det.value<< " id: " << /*det.id <<*/ "\n";
        std::cout << ss.str();
        detected.push_back(det);
    };

    auto n_blocks = n_samples / domain::BlockSize;
    for (size_t block_index = 0; block_index < n_blocks; ++block_index)
    {
        size_t sample_index = block_index * domain::BlockSize;
        auto block_time_s = block_index * domain::block_period_s; // time stamp of the first sample in this block
        auto tau_to_time_offset_s = block_time_s;
        auto signal_block = get_next_block();
        auto signal_ptr = signal_block.data();
        acquirer.run(signal_ptr, on_chirp, tau_to_time_offset_s, n_extremma, sync_period_s, sync_half_gate_s, nearby_peak_tolerance_s);


        // optional print
        if (/*0 == block_index ||*/ block_index == n_blocks-1)
        {
            std::cout << "block_time_s = " << block_time_s << "s block_index = " << block_index << "[], sample_index = " << sample_index << "[]\n";
            acquirer.extremma_helper().print(domain::sample_period_s);
            std::cout << "";
        }
    }

    auto n_detected_chirps = detected.size();
    auto n_expected_chirps = n_chirps - acquirer.nDetsForHypothesis + 1;
    ASSERT_EQ(n_expected_chirps, n_detected_chirps) << "didnt detect the right number of chirps";

    auto offset = acquirer.nDetsForHypothesis - 1; // because the first N chirps are missed
    for (size_t i = 0; i < n_chirps-offset; ++i)
    {
        auto & params_this_chirp = multi_chirp_params[i+offset];
        auto residual_s = params_this_chirp.first.center_s - detected[i].time_s;
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;
        auto residual_percentsamplerate = residual_s / domain::sample_period_s * 100.0;
        std::cout << "chirp["<<i<<"]: residual_s: " << residual_s << "s, residual_mm: " << residual_mm << "mm, residual_%samplerate: " <<residual_percentsamplerate<< "%\n";

        EXPECT_LT(std::fabs(residual_s), residual_s_tolerance);
        EXPECT_LT(std::fabs(residual_mm), residual_mm_tolerance_loose);
    }

}