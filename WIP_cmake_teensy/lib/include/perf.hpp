
#include "utils.hpp"
#include "domain.hpp"

namespace perf
{

template <typename TimePointT, typename Precision>
struct PerfResults
{
    TimePointT time_delta{}; // time_final - time_start of critical section
    Precision error{}; // should be as close to 0 as possible
};

// template <typename T>
// PerfResults(T, types::Precision) -> PerfResults<T>;

// helper function to get representative chirp and signal coefficients (using either pure c2c 
// transform with Ncoeffs == Nsamples or r2c transform with Ncoeffs == Nsamples / 2 + 1)
template <typename types, size_t Nsamples, bool r2c_else_c2c = true>
auto get_chirp_and_signal()
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;

    // chirp
    auto chirp_func = utils::sinc<Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams<Precision>{.amplitude = 1.0, .center_s = domain::sample_period_s * Nsamples * 0.5, .freq_hz = 10E3};
    auto chirp = utils::FFTHelper<types, Nsamples>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    typename types::template array_r<Nsamples> signal_input;
    for (size_t i = 0; i < signal_input.size(); ++i)
    {
        signal_input[i] = i % 17;
    }

    utils::FFTHelper<types, Nsamples> signal;
    signal.reset();
    signal.input = signal_input;
    signal.transform();

    if constexpr(r2c_else_c2c)
    {
        // pre-conjugate. gonna need it anyways
        for (auto & coeff : chirp.coeffs)
        {
            coeff = std::conj(coeff);
        }

        static_assert((Nsamples/2+1)*sizeof(Complex) == sizeof(chirp.coeffs), "expecting N/2+1 coeffs");
        return std::make_pair(chirp.coeffs, signal.coeffs);
    }
    else
    {
        typename types::template array_c<Nsamples> input_a;
        typename types::template array_c<Nsamples> input_b;
        for (size_t i = 0; i < Nsamples; ++i)
        {
            input_a[i] = Complex(chirp.input[i], 0.0);
            input_b[i] = Complex(signal.input[i], 0.0);
        }

        constexpr size_t Ncoeffs = Nsamples;
        typename types::template array_c<Ncoeffs> coeffs_a;
        typename types::template array_c<Ncoeffs> coeffs_b;

        utils::template FFT_c2c_1d<Nsamples, Real, Complex> transformer{};
        transformer.reset(-1.0);
        transformer.run(input_a.data(), coeffs_a);
        transformer.run(input_b.data(), coeffs_b);

        // pre-conjugate. gonna need it anyways
        for (auto & coeff : coeffs_a)
        {
            coeff = std::conj(coeff);
        }
        
        static_assert(Nsamples*sizeof(Complex) == sizeof(coeffs_a), "expecting N coeffs");
        return std::make_pair(coeffs_a, coeffs_b);
    }

}

// helper function to get representative chirp and signal coefficients as well as their full 
// correlation surface
template <typename types, size_t Nsamples, bool r2c_else_c2c=true>
auto get_chirp_and_signal_and_surface()
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;

    auto [c2c_coeffs_a_conj, c2c_coeffs_b] = get_chirp_and_signal<types, Nsamples, false>();

    typename types::template array_r<Nsamples> surface;

    for (size_t output_index = 0; output_index < Nsamples; ++output_index)
    {
        Real sum(0.0);
        for (size_t i = 0; i < c2c_coeffs_a_conj.size(); ++i)
        {
            auto a_conj = c2c_coeffs_a_conj[i];
            auto b = c2c_coeffs_b[i];

            Real freq = (output_index * i) / Real(domain::WindowSize);

            Complex term_i = a_conj * b * std::exp(constants::twopij * freq);

            sum += term_i.real();
        }

        surface[output_index] = sum;
    }


    auto [coeffs_a_conj, coeffs_b] = get_chirp_and_signal<types, Nsamples, r2c_else_c2c>();
    return std::make_tuple(coeffs_a_conj, coeffs_b, surface);
}

// get the maximum absolute difference between two arrays, ignoring NaN's
template <typename T, size_t N>
T get_max_abs_error(const std::array<T, N> & vecA, const std::array<T, N> & vecB)
{
    std::array<T, N> residuals{};
    T max_abs_residual = 0;
    for (size_t i = 0; i < N; ++i)
    {
        auto residual = vecA[i] - vecB[i];
        auto residual_norm = std::abs(residual);
        if (!std::isnan(residual_norm))
        {
            residuals[i] = residual_norm;
            max_abs_residual = std::max(residual_norm, max_abs_residual);
        }
    }
    return max_abs_residual;
}


// example to show how to create a benchmark
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto example(const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;

    auto time_start = time_func();
    
    print_stream << "party hard\n";

    auto time_stop = time_func();

    auto error = Precision{0.0};
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}

// fully naive implementation. 
// compute std::exp live (no pre-compute)
// multiply all coefficients, N*N operations
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto naive_c2c_live_exp(const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;

    constexpr size_t Nsamples = domain::WindowSize;

    auto [coeffs_a_conj, coeffs_b, surface_true] = get_chirp_and_signal_and_surface<types, Nsamples, false>();
    // constexpr size_t Ncoeffs = Nsamples;

    typename types::template array_r<Nsamples> surface;

    auto time_start = time_func();

    for (size_t output_index = 0; output_index < Nsamples; ++output_index)
    {
        Real sum(0.0);
        for (size_t i = 0; i < coeffs_a_conj.size(); ++i)
        {
            auto a_conj = coeffs_a_conj[i];
            auto b = coeffs_b[i];

            Real freq = output_index * i / static_cast<Real>(domain::WindowSize);

            Complex term_i = a_conj * b * std::exp(constants::twopij * freq);

            sum += term_i.real();
        }

        surface[output_index] = sum;
    }

    auto time_stop = time_func();

    auto error = get_max_abs_error(surface, surface_true);
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}

// still naive implementation
// compute std::exp live  (no pre-compute)
// use half the coefficients by leveraging conjugate symmetry (N/2+1 insead of N)
// reduce that number more since many chirp coefficients are ~0
// use half the outputs since we dont need them
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto naive_r2c_live_exp(const TimeFuncT & time_func, PrintStreamT & print_stream, double percent_threshold = -0.1)
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;

    constexpr size_t Nsamples = domain::WindowSize;
    
    auto [coeffs_a_conj, coeffs_b, surface_true] = get_chirp_and_signal_and_surface<types, Nsamples>();
    constexpr size_t Ncoeffs = utils::Nsamples_to_Ncoeffs(Nsamples);
    
    // double the coefficients. but skip DC and Nyquist, they stand alone
    for (size_t i = 1; i < coeffs_a_conj.size()-1; ++i)
    {
        coeffs_a_conj[i] *= 2;
    }
    
    // search for the number of coefficients to use. we are not using the negative frequencies because they are assumed to cancel
    size_t Ncoeffs_to_multiply = 0;
    {
        Precision norm_dc = std::abs(coeffs_a_conj[0]);
        Precision norm_nyquist = std::abs(coeffs_a_conj[coeffs_a_conj.size()-1]);
        auto norm_delta = (norm_dc - norm_nyquist);
        for (size_t i = 0; i < Ncoeffs; ++i)
        {
            if ((std::abs(coeffs_a_conj[i]) - norm_nyquist) / norm_delta > percent_threshold)
            {
                ++Ncoeffs_to_multiply; 
            }
        }
    }

    print_stream << "Ncoeffs_to_multiply: " << Ncoeffs_to_multiply << "\n";

    typename types::template array_r<Nsamples> surface;
    surface.fill(std::numeric_limits<Precision>::quiet_NaN());

    auto time_start = time_func();

    constexpr size_t Noutputs_used = Nsamples/2;
    for (size_t output_index = 0; output_index < Noutputs_used; ++output_index)
    {
        Real sum(0.0);
        for (size_t i = 0; i < Ncoeffs; ++i)
        {
            auto a_conj = coeffs_a_conj[i];
            auto b = coeffs_b[i];

            Real freq = output_index * i / Real(domain::WindowSize);

            Complex term_i = a_conj * b * std::exp(constants::twopij * freq);

            sum += term_i.real();
        }

        surface[output_index] = sum;
    }

    auto time_stop = time_func();
    
    auto error = get_max_abs_error(surface, surface_true);
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}

// still naive implementation
// pre-compute std::liveexp
// use half the coefficients by leveraging conjugate symmetry
// use all chirp coefficients, even those near 0
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto naive_r2c_precompute_exp(const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;

    constexpr size_t Nsamples = domain::WindowSize;

    auto [coeffs_a_conj, coeffs_b, surface_true] = get_chirp_and_signal_and_surface<types, Nsamples>();
    constexpr size_t Ncoeffs = utils::Nsamples_to_Ncoeffs(Nsamples);
    
    // double the coefficients. but skip DC and Nyquist, they stand alone
    for (size_t i = 1; i < coeffs_a_conj.size()-1; ++i)
    {
        coeffs_a_conj[i] *= 2;
    }

    typename types::template array_r<Nsamples> surface;
    surface.fill(std::numeric_limits<Real>::quiet_NaN());

    std::array<typename types::template array_c<Ncoeffs>, Nsamples> Wn;
    for (size_t output_index = 0; output_index < Nsamples; ++output_index)
    {
        for (size_t i = 0; i < Ncoeffs; ++i)
        {
            Real freq = (output_index * i) / Real(domain::WindowSize);
            auto a_conj = coeffs_a_conj[i];
            Wn[output_index][i] = a_conj * std::exp(constants::twopij * freq);
        }
    }

    auto time_start = time_func();

    constexpr size_t Noutputs_used = Nsamples/2;
    for (size_t output_index = 0; output_index < Noutputs_used; ++output_index)
    {
        Real sum(0.0);
        for (size_t i = 0; i < Ncoeffs; ++i)
        {
            auto b = coeffs_b[i];

            Complex term_i =  b * Wn[output_index][i]; //a_conj baked in

            sum += term_i.real();
        }

        surface[output_index] = sum;
    }

    auto time_stop = time_func();

    auto error = get_max_abs_error(surface, surface_true);
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}

// efficient radix2 implementation
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto fft_r2c_radix2(const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;

    constexpr size_t Nsamples = domain::WindowSize;

    auto [coeffs_a_conj, coeffs_b, surface_true] = get_chirp_and_signal_and_surface<types, Nsamples>();
    constexpr size_t Ncoeffs = utils::Nsamples_to_Ncoeffs(Nsamples);
    
    // no need to double the coefficients. c2r does that for us by its design

    utils::FFT_real_1d<Nsamples, Real, Complex> transformer{};

    transformer.reset(+1.0);
    typename types::template array_c<Ncoeffs> a_conj_b;
    typename types::template array_r<Nsamples> surface;

    auto time_start = time_func();

    for (size_t i = 0; i < Ncoeffs; ++i)
    {
        a_conj_b[i] = coeffs_a_conj[i] * coeffs_b[i];
    }

    transformer.c2r(a_conj_b, surface);
    // transformer.rescale(surface);

    auto time_stop = time_func();

    auto error = get_max_abs_error(surface, surface_true);
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}

// naive non-integer evaluation
// compute std::exp live for each coefficient in each derivative order (Nsamples*O)
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto noninteger_evaluation_v0(const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;
    using FFTHelperT = utils::FFTHelper<types, domain::WindowSize>;

    // chirp
    auto chirp_func = utils::sinc<Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams<Precision>{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = FFTHelperT::template construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = utils::get_offset_params<Precision>(domain::sample_period_s, chirp_params, offset_s);
    auto signal = FFTHelperT::template construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::CorrelationHelper<types, domain::WindowSize, 2> correlation_helper;
    correlation_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    auto correlate_and_derive = [&]<size_t derivative_order>(auto tau)
    {
        return correlation_helper.template correlate_and_derive_v0<derivative_order>(signal.coeffs, tau);
    };

    auto fd0_fd1_fd2 = [&](auto tau)
    { return correlate_and_derive.template operator()<2>(tau); };

    auto time_start = time_func();

    Precision guessed_tau_s = 0 + 0.5 * domain::sample_period_s;
    auto [optimal_tau_s, optimal_value] = utils::newton(fd0_fd1_fd2, guessed_tau_s);

    auto time_stop = time_func();

    auto residual_s = (tau_true_s - optimal_tau_s);
    // auto residual_mm = constants::speed_of_sound_mmps * residual_s;
    
    auto error = residual_s;
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}

// less naive non-integer evaluation 
// compute std::exp live for each coefficient, but reuse for each derivative order (N)
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto noninteger_evaluation_v1(const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;
    using FFTHelperT = utils::FFTHelper<types, domain::WindowSize>;

    // chirp
    auto chirp_func = utils::sinc<Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams<Precision>{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = FFTHelperT::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = utils::get_offset_params<Precision>(domain::sample_period_s, chirp_params, offset_s);
    auto signal = FFTHelperT::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::CorrelationHelper<types, domain::WindowSize, 2> correlation_helper;
    correlation_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    auto correlate_and_derive = [&]<size_t derivative_order>(auto tau)
    {
        return correlation_helper.template correlate_and_derive_v1<derivative_order>(signal.coeffs, tau);
    };

    auto fd0_fd1_fd2 = [&](auto tau)
    { return correlate_and_derive.template operator()<2>(tau); };

    auto time_start = time_func();

    Precision guessed_tau_s = 0 + 1.5 * domain::sample_period_s;
    auto [optimal_tau_s, optimal_value] = utils::newton(fd0_fd1_fd2, guessed_tau_s);

    auto time_stop = time_func();

    auto residual_s = (tau_true_s - optimal_tau_s);
    // auto residual_mm = constants::speed_of_sound_mmps * residual_s;

    auto error = residual_s;
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}

// even less naive non-integer evalutaion
// compute std::exp once and tweak it for each coefficient, reused for each derivative order (1+multiplies)
template <typename types, typename TimeFuncT, typename PrintStreamT>
auto noninteger_evaluation_v2(const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;
    using Real = typename types::real_type;
    using Complex = typename types::complex_type;
    using domain = Domain<Precision>;
    using constants = Constants<Real, Complex>;
    using FFTHelperT = utils::FFTHelper<types, domain::WindowSize>;

    // chirp
    auto chirp_func = utils::sinc<Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams<Precision>{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = FFTHelperT::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = utils::get_offset_params<Precision>(domain::sample_period_s, chirp_params, offset_s);
    auto signal = FFTHelperT::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::CorrelationHelper<types, domain::WindowSize, 2> correlation_helper;
    correlation_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    auto correlate_and_derive = [&]<size_t derivative_order>(auto tau)
    {
        return correlation_helper.template correlate_and_derive_v2<derivative_order>(signal.coeffs, tau);
    };

    auto fd0_fd1_fd2 = [&](auto tau)
    { return correlate_and_derive.template operator()<2>(tau); };

    auto time_start = time_func();

    Precision guessed_tau_s = 0 + 1.5 * domain::sample_period_s;
    auto [optimal_tau_s, optimal_value] = utils::newton(fd0_fd1_fd2, guessed_tau_s);

    auto time_stop = time_func();

    auto residual_s = (tau_true_s - optimal_tau_s);
    // auto residual_mm = constants::speed_of_sound_mmps * residual_s;
    // print_stream << "residual: " << residual_s << "s, " << residual_mm << "mm\n";

    auto error = residual_s;
    
    return PerfResults{.time_delta=(time_stop - time_start), .error = error};
}


template <typename TimeFuncT, typename PrintStreamT, typename QueryFuncT, typename ... Args>
void run_test_impl(size_t n_trials, const TimeFuncT & time_func, PrintStreamT & print_stream, const std::string& name, double err_thresh, QueryFuncT && query_func, Args && ... args)
{
    // constexpr size_t n_trials =  100;

    print_stream << "---- " << name << " ----\n";

    using DurationT = decltype(time_func() - time_func());
    
    DurationT total_duration{};
    size_t error_count{};

    for (size_t i = 0; i < n_trials; ++i)
    {
        print_stream.permit_writes = (i == 0);
        auto results = query_func(time_func, print_stream, std::forward<Args>(args)...);
        print_stream.permit_writes = true;

        if (std::abs(results.error) > err_thresh)
        {
            print_stream << "ERROR: std::abs("<<results.error<<") > "<<err_thresh<<"\n";
            ++error_count;
        }
        total_duration += /*std::chrono::duration_cast<std::chrono::microseconds>*/(results.time_delta);
    }
    print_stream << "run time: " << total_duration/*.count()*//(1.0*n_trials) << "μs, " << error_count << " errors\n";
    print_stream << "\n";
}

struct NullStream
{
    template <typename T>
    NullStream& operator<<(const T&)
    {
        return *this;
    }
};

template <typename StreamT>
struct StreamWrapper
{
    bool permit_writes = true;
    StreamT& stream;

    template <typename T>
    StreamWrapper& operator<<(const T& value)
    {
        if (permit_writes)
        {
            stream << value;
        }

        return *this;
    }
};

template <typename StreamT>
StreamWrapper(bool, StreamT&) -> StreamWrapper<StreamT>;

template <typename types, typename TimeFuncT, typename PrintStreamT>
void run_performance_suite(size_t n_trials, const TimeFuncT & time_func, PrintStreamT & print_stream)
{
    using Precision = typename types::precision_type;

    auto run_test = [&]<typename ... Args>(Args && ... args)
    {
        run_test_impl(n_trials, time_func, print_stream, std::forward<Args>(args)...);
    };

    run_test("example", 
              0.0,
             example<types, TimeFuncT, PrintStreamT>
            );

    
    Precision max_fft_agreement_error = 1E-6;

    print_stream << "#\n# benchmarks for evaluating the discrete fourier transform (DFT) at integer locations:\n#\n";

    run_test("naive_c2c_live_exp", 
              max_fft_agreement_error,
             naive_c2c_live_exp<types, TimeFuncT, PrintStreamT>
            );

    run_test("naive_r2c_live_exp", 
             max_fft_agreement_error,
             naive_r2c_live_exp<types, TimeFuncT, PrintStreamT>,
             /*coeff_norm_thresh_to_use*/ -1.0
            );

    run_test("naive_r2c_live_exp", 
             max_fft_agreement_error,
             naive_r2c_live_exp<types, TimeFuncT, PrintStreamT>,
             /*coeff_norm_thresh_to_use*/ 0.1
            );

    // run_test("naive_r2c_precompute_exp", 
    //          max_fft_agreement_error,
    //          naive_r2c_precompute_exp<types, TimeFuncT, PrintStreamT>
    //         );

    run_test("fft_r2c_radix2", 
             max_fft_agreement_error,
             fft_r2c_radix2<types, TimeFuncT, PrintStreamT>
            );


    print_stream << "#\n# benchmarks for evaluating the discrete fourier transform (DFT) at non-integer locations:\n#\n";

    Precision max_correlation_peak_location_error = 1E-6;

    run_test("noninteger_evaluation_v0", 
            max_correlation_peak_location_error,
             noninteger_evaluation_v0<types, TimeFuncT, PrintStreamT>);

    run_test("noninteger_evaluation_v1", 
            max_correlation_peak_location_error,
             noninteger_evaluation_v1<types, TimeFuncT, PrintStreamT>);

    run_test("noninteger_evaluation_v2", 
            max_correlation_peak_location_error,
             noninteger_evaluation_v2<types, TimeFuncT, PrintStreamT>);

}

}