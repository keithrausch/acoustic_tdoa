
#include <gtest/gtest.h>
#include <random>

#include <fftw3.h>
#include <fftw3.h>
#include <Eigen/Dense>

#include "domain.hpp"
#include "utils.hpp"

constexpr types::Precision residual_s_tolerance = 0.5 * domain::sample_period_s;
constexpr types::Precision residual_mm_tolerance_tight = 1.0;
constexpr types::Precision residual_mm_tolerance_loose = 2.0;

template <typename T>
void assert_eq(const T& arrA, const T& arrB, double abs_tol = 1E-6, double rel_tol = 1E-6/*1E-12*/)
{
    for (size_t i = 0; i < arrA.size(); ++i)
    {
        const auto a = arrA[i];
        const auto b = arrB[i];

        const auto abs_error = std::abs(a - b);
        const auto scale = std::max(std::abs(a), std::abs(b));

        const auto tolerance =
            abs_tol + rel_tol * scale;

        ASSERT_LE(abs_error, tolerance)
            << "i = " << i
            << ", a = " << a
            << ", b = " << b
            << ", abs_error = " << abs_error
            << ", tolerance = " << tolerance
            << ", rel_error = "
            << (scale > 0 ? abs_error / scale : 0.0);
    }
}

template <size_t N>
constexpr auto combinations_of_N_choose_2()
{
    constexpr size_t nCr = utils::nCr(N, 2);
    std::array<std::pair<size_t, size_t>, nCr> ret;
    size_t ret_index = 0;
    for (size_t i = 0; i < N-1; ++i)
    {
        for (size_t j = i+1; j < N; ++j)
        {
            ret[ret_index++] = std::make_pair(i, j);
        }
    }

    return ret;
}

template <size_t GroupSize, size_t NumSets >
constexpr auto sets_of_combinations_of_N_choose_2()
{
    constexpr size_t nCr = utils::nCr(GroupSize, 2);
    std::array<std::pair<size_t, size_t>, nCr*NumSets> ret;

    constexpr auto impl = combinations_of_N_choose_2<GroupSize>();
    
    size_t ret_index = 0;
    for (size_t i = 0; i < NumSets; ++i)
    {
        for (const auto [a, b] : impl)
        {
            ret[ret_index].first = a+i*GroupSize;
            ret[ret_index].second = b+i*GroupSize;
            ++ret_index;
        }
    }

    return ret;
}

TEST(CalibrationTest, CalibrationTest)
{
    constexpr size_t group_size = 4;
    constexpr size_t N_groups_m = 1; // mics
    constexpr size_t N_groups_s = 2; // speakers

    constexpr size_t Nm = group_size*N_groups_m; // mics
    constexpr size_t Ns = group_size*N_groups_s; // speakers

    typedef Eigen::Vector<types::Precision, 3> Vector3p;

    std::array<Vector3p, Ns> pos_s;
    std::array<Vector3p, Nm> pos_m;

    // constexpr size_t nCr_s = utils::nCr(Ns, 2);
    // constexpr size_t nCr_m = utils::nCr(Nm, 2);

    // generate data
    std::random_device rd; 
    std::mt19937 gen(rd());
    std::uniform_real_distribution<types::Precision> dis(-1.0, +1.0);
    
    
    pos_m[0] = Vector3p(1,1,1) * 2;
    pos_m[1] = Vector3p(1,0,0) * 2;
    pos_m[2] = Vector3p(0,1,0) * 2;
    pos_m[3] = Vector3p(0,0,1) * 2;
    for (size_t i = 4; i < pos_m.size(); ++i)
    {
        pos_m[i] = Vector3p(dis(gen), dis(gen), dis(gen));
    }

    pos_s[0] = Vector3p(0,0,0);
    pos_s[1] = Vector3p(1,0,0);
    pos_s[2] = Vector3p(0,1,0);
    pos_s[3] = Vector3p(0,0,1);
    for (size_t i = 4; i < pos_s.size(); ++i)
    {
        pos_s[i] = Vector3p(dis(gen), dis(gen), dis(gen));
    }

    constexpr auto combos_m = sets_of_combinations_of_N_choose_2<group_size, N_groups_m>();
    constexpr auto combos_s = sets_of_combinations_of_N_choose_2<group_size, N_groups_s>();

    std::array<std::array<types::Precision, Nm>, Ns> measurement_times;

    for (size_t s = 0; s < Ns; ++s)
    {
        for (size_t m = 0; m < Nm; ++m)
        {
            types::Precision distance = (pos_s[s]-pos_m[m]).norm(); // meters
            measurement_times[s][m] = distance * constants::speed_of_sound_spm;
        }
    }

    constexpr bool verbose = false;
    if (verbose)
    {
        for (size_t s = 0; s < Ns; ++s)
        {
            for (size_t m = 0; m < Nm; ++m)
            {
                std::cout << std::fixed << std::setprecision(5) << measurement_times[s][m] << ", ";
            }
            std::cout << "\n";
        }
    }

    constexpr size_t Nconstraints = 3 + 2 + 1; // first speaker at [0,0,0], second speaker along the x-axis, third speaker in the z=0 plane
    constexpr size_t Ntdoa = combos_m.size()*Ns + combos_s.size()*Nm;
    constexpr size_t Nstates = 3*(Ns + Nm); // total number of unknowns, but without constraints
    constexpr size_t jac_Nrows = Ntdoa+Nconstraints;
    constexpr size_t jac_Ncols = Nstates;
    typedef Eigen::Vector<types::Precision, 3> Vec;
    typedef Eigen::Vector<types::Precision, Nstates> States;
    typedef Eigen::Vector<types::Precision, jac_Nrows> Meas;
    typedef Eigen::Matrix<types::Precision, jac_Nrows, jac_Ncols> Jac;
    Meas measurements_true;

    static_assert(jac_Nrows >= jac_Ncols, "jacobian not square, not enough measurements or constraints");
    static_assert(Nstates == jac_Ncols, "you messed something up. the matrix dimensions dont match between number of states and columns of jacobian");


    //
    // construct measurement vector
    //
    {
        size_t meas_index = 0;

        // every combination of mics given a specific speaker
        for (size_t s_index = 0; s_index < Ns; ++s_index)
        {
            for (const auto [a, b] : combos_m)
            {
                measurements_true[meas_index++] = measurement_times[s_index][a] - measurement_times[s_index][b];
            }
        }

        // every combination of speakers given a specific mic
        for (size_t m_index = 0; m_index < Nm; ++m_index)
        {
            for (const auto [a, b] : combos_s)
            {
                measurements_true[meas_index++] = measurement_times[a][m_index] - measurement_times[b][m_index];
            }
        }

        // first speaker at [0,0,0]
        measurements_true[meas_index++] = 0;
        measurements_true[meas_index++] = 0;
        measurements_true[meas_index++] = 0;

        // second speaker at [?,0,0]
        // measurements_true[meas_index++] = 0;
        measurements_true[meas_index++] = 0;
        measurements_true[meas_index++] = 0;

        // third speaker at [?,?,0]
        // measurements_true[meas_index++] = 0;
        // measurements_true[meas_index++] = 0;
        measurements_true[meas_index++] = 0;
    }

    // fill in the estimates of all states
    States states_true;
    for (size_t m_index = 0; m_index < Nm; ++m_index)
    {
        states_true.block<3, 1>(3*m_index, 0) = pos_m[m_index];
    }
    for (size_t s_index = 0; s_index < Ns; ++s_index)
    {
        states_true.block<3, 1>(3*Nm + 3*s_index, 0) = pos_s[s_index];
    }

    States states_est = states_true;
    states_est[0] = 5;
    Meas measurements_est;

    std::unique_ptr<Jac> jac_ptr = std::make_unique<Jac>();//Jac::Zero();// * std::numeric_limits<types::Precision>::quiet_NaN();
    auto & jac = *jac_ptr;
    jac *= 0.0;

    auto update_F_J = [&](const States &states_est)
    {
        size_t meas_index = 0;

        // every combination of mics given a specific speaker
        for (size_t s_index = 0; s_index < Ns; ++s_index)
        {
            for (const auto [a, b] : combos_m)
            {
                Vec mic_a = states_est.block<3,1>(3*a,0); // pos_m[a];
                Vec mic_b = states_est.block<3,1>(3*b,0); // pos_m[b];
                Vec speak = states_est.block<3,1>(3*Nm + 3*s_index, 0); // pos_s[s_index];

                Vec delta_a = mic_a - speak;
                Vec delta_b = mic_b - speak;
                types::Precision norm_squared_a = delta_a.transpose() * delta_a;
                types::Precision norm_squared_b = delta_b.transpose() * delta_b;
                types::Precision norm_a = std::sqrt(norm_squared_a);
                types::Precision norm_b = std::sqrt(norm_squared_b);

                auto val = (delta_a.norm() - delta_b.norm())*constants::speed_of_sound_spm - measurements_true[meas_index];
                measurements_est[meas_index] = val;

                types::Precision inv_norm_a = 1.0 / norm_a;
                types::Precision inv_norm_b = 1.0 / norm_b;
                Vec J_ma = inv_norm_a * delta_a.transpose() * constants::speed_of_sound_spm;
                Vec J_mb = inv_norm_b * delta_b.transpose() * -1 * constants::speed_of_sound_spm;
                Vec J_s  = -1 * (J_ma + J_mb);
                jac.block<1,3>(meas_index, 3*a) = J_ma;
                jac.block<1,3>(meas_index, 3*b) = J_mb;
                jac.block<1,3>(meas_index, 3*Nm + 3*s_index) = J_s;

                ++meas_index;
            }
        }


        // every combination of speakers given a specific mic
        for (size_t m_index = 0; m_index < Nm; ++m_index)
        {
            for (const auto [a, b] : combos_s)
            {
                Vec speak_a = states_est.block<3,1>(3*Nm+3*a, 0);// pos_s[a];
                Vec speak_b = states_est.block<3,1>(3*Nm+3*b, 0);// pos_s[b];
                Vec mic = states_est.block<3,1>(3*m_index, 0);// pos_m[m_index];

                Vec delta_a = speak_a - mic;
                Vec delta_b = speak_b - mic;
                types::Precision norm_squared_a = delta_a.transpose() * delta_a;
                types::Precision norm_squared_b = delta_b.transpose() * delta_b;
                types::Precision norm_a = std::sqrt(norm_squared_a);
                types::Precision norm_b = std::sqrt(norm_squared_b);
                
                auto val = (delta_a.norm() - delta_b.norm())*constants::speed_of_sound_spm - measurements_true[meas_index];
                measurements_est[meas_index] = val;

                types::Precision inv_norm_a = 1.0 / norm_a;
                types::Precision inv_norm_b = 1.0 / norm_b;
                Vec J_sa = inv_norm_a * delta_a.transpose() * constants::speed_of_sound_spm;
                Vec J_sb = inv_norm_b * delta_b.transpose() * -1 * constants::speed_of_sound_spm;
                Vec J_m  = -1 * (J_sa + J_sb);
                jac.block<1,3>(meas_index, 3*Nm + 3*a) = J_sa;
                jac.block<1,3>(meas_index, 3*Nm + 3*b) = J_sb;
                jac.block<1,3>(meas_index, 3*m_index) = J_m;

                ++meas_index;
            }
        }

        // measurements for the constraints
        // first speaker at x=0, y=0, z=0
        {
            size_t i = 3*Nm+3*0;
            // x
            measurements_est[meas_index] = states_est[i+0]- measurements_true[meas_index];
            jac(meas_index, i+0) = 1;
            ++meas_index;
            // y
            measurements_est[meas_index] = states_est[i+1]- measurements_true[meas_index];
            jac(meas_index, i+1) = 1;
            ++meas_index;
            // z
            measurements_est[meas_index] = states_est[i+2]- measurements_true[meas_index];
            jac(meas_index, i+2) = 1;
            ++meas_index;
        }

        // second speaker on x-axis (y=0, z=0)
        {
            size_t i = 3*Nm+3*1;
            // y
            measurements_est[meas_index] = states_est[i+1]- measurements_true[meas_index];
            jac(meas_index, i+1) = 1;
            ++meas_index;
            // z
            measurements_est[meas_index] = states_est[i+2]- measurements_true[meas_index];
            jac(meas_index, i+2) = 1;
            ++meas_index;
        }

        // third speaker in x-y-plane (z=0)
        {
            size_t i = 3*Nm+3*2;
            // z
            measurements_est[meas_index] = states_est[i+2]- measurements_true[meas_index];
            jac(meas_index, i+2) = 1;
            ++meas_index;
        }

    };

    if (verbose)
    {
        std::cout << "jac size: (" << jac_Nrows << ", " << jac_Ncols << ")\n";
        std::cout << "jac:\n" << jac << "\n";
    }

    auto options = (Eigen::ComputeFullU | Eigen::ComputeFullV);
    // auto solver_ptr = std::make_unique<Eigen::CompleteOrthogonalDecomposition<Jac>>(jac/*, options*/);
    auto solver = Eigen::CompleteOrthogonalDecomposition<Jac>(jac/*, options*/);

    for (size_t i = 0; i < 100; ++i)
    {
        update_F_J(states_est);
        // Jac jac_orig = jac;

        // Jac jac_diffed;
        // for (size_t ii = 0; ii < jac_Ncols; ++ii)
        // {
        //     States x_f = states_est;
        //     States x_b = states_est;
        //     double h = 0.000001;
        //     x_f[ii] += h;
        //     x_b[ii] -= h;

        //     update_F_J(x_f);
        //     jac_diffed.col(ii) = measurements_est;
        //     update_F_J(x_b);
        //     jac_diffed.col(ii) -= measurements_est;
        //     jac_diffed.col(ii) /= (2*h);
        // }

        // std::cout << "delta:\n" << jac_diffed - jac_orig << "\n";
        // jac = jac_diffed;



        // apply update
        solver.compute(jac);
        States update = solver.solve(measurements_est);
        if (verbose)
        {
            std::cout << "meas_est: " << measurements_est.transpose() << "\n";
            std::cout << "states_est: " << states_est.transpose() << "\n";
            std::cout << "update: (norm=" << update.norm() << "), " << update.transpose() << "\n";
        }

        Eigen::FullPivLU<Jac> lu_decomp(jac);
        auto rank = lu_decomp.rank();
        if (verbose)
        {
            std::cout << "jac size: (" << jac_Nrows << ", " << jac_Ncols << "), rank: "<< rank << "\n";
            std::cout << "\n";
        }
        
        states_est -= update;

        // if (update.norm() < 0.001)
        // {
        //     break;
        // }
    }

    States states_residual = states_true - states_est;

    if (verbose)
    {
        std::cout << "residual: (norm=" << states_residual.norm() << "), " << states_residual.transpose() << "\n";
    }
}

template <size_t Nsamples>
void run_c2c_test_for_Nsamples()
{
    // generate data
    std::random_device rd; 
    std::mt19937 gen(rd());
    std::uniform_real_distribution<types::Precision> dis(-1.0, +1.0);
    types::array_cp<Nsamples> input;
    for (auto & element : input)
    {
        element = types::cPrecision(dis(gen), dis(gen));
    }

    utils::FFT_c2c_1d<Nsamples> forward;
    forward.reset();

    // run new method
    typename utils::FFT_c2c_1d<Nsamples>::OutputT coeffs;
    coeffs.fill(0);
    forward.run(input.data(), coeffs);


    utils::FFT_c2c_1d<Nsamples> inverse;
    inverse.reset(+1.0);

    // run new method
    typename utils::FFT_c2c_1d<Nsamples>::OutputT reconstruct;
    reconstruct.fill(0);
    inverse.run(coeffs.data(), reconstruct);
    inverse.rescale(reconstruct);

    ASSERT_EQ(coeffs.size(), input.size());
    ASSERT_EQ(coeffs.size(), reconstruct.size());

    assert_eq(input, reconstruct);
}

#define TEST_FFT_C2C(NAME, N)              \
TEST(FFTTest, FFT_C2C_##NAME)              \
{                                          \
    run_c2c_test_for_Nsamples<N>();        \
}

TEST_FFT_C2C(N1, 1)
TEST_FFT_C2C(N2, 2)
TEST_FFT_C2C(N4, 4)
TEST_FFT_C2C(N8, 8)
TEST_FFT_C2C(N16, 16)
TEST_FFT_C2C(N32, 32)
TEST_FFT_C2C(N64, 64)
TEST_FFT_C2C(N128, 128)
TEST_FFT_C2C(N256, 256)
TEST_FFT_C2C(BlockSize, domain::BlockSize)
TEST_FFT_C2C(WindowSize, domain::WindowSize)

template <size_t Nsamples>
void run_r2c_test_for_Nsamples()
{
    // generate data
    std::random_device rd; 
    std::mt19937 gen(rd());
    std::uniform_real_distribution<types::Precision> dis(-1.0, +1.0);
    types::array_p<Nsamples> input;
    for (size_t i = 0; i < input.size(); ++i)
    {
        input[i] = i; //dis(gen);
    }

    utils::FFT_real_1d<Nsamples> fft_forward;

    // run new method
    typename utils::FFT_real_1d<Nsamples>::CoeffsT manual_coeffs;
    manual_coeffs.fill(0);
    fft_forward.r2c(input, manual_coeffs);

    constexpr static size_t Ncoeffs = utils::Nsamples_to_Ncoeffs(Nsamples);
    typedef types::array_cp<Ncoeffs> CoeffsT; // elements are 2 doubles, so we need half the length
    typedef types::array_p<Nsamples> RealsT;
    
    types::array_cp<Ncoeffs> fftw_coeffs; // fftw needs 1 extra element when executing
    if constexpr(std::is_same_v<types::Precision, double>)
    {
        fftw_plan plan{};
        plan = fftw_plan_dft_r2c_1d(Nsamples, input.data(), reinterpret_cast<fftw_complex *>(fftw_coeffs.data()), FFTW_ESTIMATE);
        fftw_execute(plan);
        fftw_destroy_plan(plan);
    }
    else if constexpr(std::is_same_v<types::Precision, float>)
    {
        fftwf_plan plan{};
        plan = fftwf_plan_dft_r2c_1d(Nsamples, input.data(), reinterpret_cast<fftwf_complex *>(fftw_coeffs.data()), FFTW_ESTIMATE);
        fftwf_execute(plan);
        fftwf_destroy_plan(plan);
    }

    ASSERT_EQ(fftw_coeffs.size(), manual_coeffs.size());

    types::Precision total_error = 0;
    for (size_t i = 0; i < Ncoeffs; ++i)
    {
        // std::cout << chirp.coeffs[i] << ", " << coeffs[i] << "\n";
        auto this_error = std::abs(fftw_coeffs[i] - manual_coeffs[i]);
        total_error += this_error * this_error;
        // std::cout << fftw_coeffs[i] << ", " << manual_coeffs[i] << "\n";
    }
    total_error = std::sqrt(total_error);
    ASSERT_LT(total_error, 1E-10);

    // test reconstruction
    {

        types::array_p<Nsamples> reconstructed;
        reconstructed.fill(0);

        utils::FFT_real_1d<Nsamples> fft_inverse(+1);
        fft_inverse.c2r(manual_coeffs, reconstructed);
        fft_inverse.rescale(reconstructed);

        ASSERT_EQ(input.size(), reconstructed.size());

        types::Precision total_error = 0;
        for (size_t i = 0; i < Ncoeffs; ++i)
        {
            // std::cout << chirp.coeffs[i] << ", " << coeffs[i] << "\n";
            auto this_error = std::abs(input[i] - reconstructed[i]);
            total_error += this_error * this_error;
            // std::cout << fftw_coeffs[i] << ", " << manual_coeffs[i] << "\n";
        }
        total_error = std::sqrt(total_error);
        ASSERT_LT(total_error, 1E-10);
    }
}

#define TEST_FFT_R2C(NAME, N)              \
TEST(FFTTest, FFT_R2C_##NAME)              \
{                                          \
    run_r2c_test_for_Nsamples<N>();        \
}

TEST_FFT_R2C(N1, 1)
TEST_FFT_R2C(N2, 2)
TEST_FFT_R2C(N4, 4)
TEST_FFT_R2C(N8, 8)
TEST_FFT_R2C(N16, 16)
TEST_FFT_R2C(N32, 32)
TEST_FFT_R2C(N64, 64)
TEST_FFT_R2C(N128, 128)
TEST_FFT_R2C(N256, 256)
TEST_FFT_R2C(BlockSize, domain::BlockSize)
TEST_FFT_R2C(WindowSize, domain::WindowSize)


template <size_t derivative_order>
void test_correlation_helper_on_derivative_order_over_full_window()
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 10E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    types::array_p<domain::WindowSize> signal_input;
    for (size_t i = 0; i < signal_input.size(); ++i)
    {
        signal_input[i] = i % 17;
    }

    utils::FFTHelper<domain::WindowSize> signal;
    signal.reset();
    signal.input = signal_input;
    signal.transform();


    utils::CorrelationHelper<domain::WindowSize, derivative_order> correlation_helper;
    correlation_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    {
        correlation_helper.template set_correlation_surface_via_fft<derivative_order>(signal.coeffs);

        types::Precision tau_s_lower_bound = 0.0; // -1.0 /* whole window */ * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_upper_bound = +2.0 /* whole window */ * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_step = domain::sample_period_s;

        // the chirp was centered at half the window duration, so the 0th index in the fft outputs
        // should correspond to tau of 0
        types::array_p<domain::WindowSize> nsquared_results;
        auto tau_s = tau_s_lower_bound;
        for (size_t i = 0; i < nsquared_results.size(); ++i)
        {
            nsquared_results[i] = correlation_helper.template correlate_and_derive<derivative_order>(signal.coeffs, tau_s)[derivative_order];
            tau_s += tau_s_step;
        }

        types::array_p<domain::WindowSize> resid;
        for (size_t i = 0; i < resid.size(); ++i )
        {
            resid[i] = std::abs(correlation_helper.correlation_surface[derivative_order][i] - nsquared_results[i]);
        }

        // utils::print("resid", resid);
        assert_eq(correlation_helper.correlation_surface[derivative_order], nsquared_results);
    }
}

template <size_t derivative_order>
void test_correlation_helper_on_derivative_order_over_mid_window()
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 10E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    types::array_p<domain::WindowSize> signal_input;
    for (size_t i = 0; i < signal_input.size(); ++i)
    {
        signal_input[i] = i % 17;
    }

    utils::FFTHelper<domain::WindowSize> signal;
    signal.reset();
    signal.input = signal_input;
    signal.transform();


    utils::CorrelationHelper<domain::WindowSize, derivative_order> correlation_helper;
    correlation_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    {
        correlation_helper.template set_correlation_surface_via_fft<derivative_order>(signal.coeffs);

        types::Precision tau_s_lower_bound = -0.5 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_upper_bound = +0.5 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_step = domain::sample_period_s;

        types::array_p<domain::BlockSize> nsquared_results;
        auto tau_s = tau_s_lower_bound;
        for (size_t i = 0; i < nsquared_results.size(); ++i)
        {
            nsquared_results[i] = correlation_helper.template correlate_and_derive<derivative_order>(signal.coeffs, tau_s)[derivative_order];
            tau_s += tau_s_step;
        }

        // we are sweeping from -tau to +tau, but doing half of the window. since the 0th index of 
        // the fft results corresponds to a tau of 0, we need to the back quarter of the window 
        // (negative tau) and then concatenate that with the first quarter of the window (positive 
        // tau)
        types::array_p<domain::BlockSize> fft_results_reordered;
        size_t i = 0;

        // this is the "negative tau" side of the results. all the way from most negative to 0
        for (size_t j = 0; j < domain::BlockSize/2; ++j)
        {
            auto offset = domain::BlockSize + domain::BlockSize/2;
            fft_results_reordered[i++] = correlation_helper.correlation_surface[derivative_order][offset+j];
        }

        // this is the "positive tau" side of the results. all the way from 0 to most positive
        for (size_t j = 0 ; j < domain::BlockSize/2; ++j)
        {
            fft_results_reordered[i++] = correlation_helper.correlation_surface[derivative_order][j];
        }

        // check each element is relatively close to truth
        assert_eq(fft_results_reordered, nsquared_results);
    }
    // fft.rescale(butterfly_results);
}


TEST(FFTTest, CorrelationHelperViaFFT_order0_full_window_Test)
{
    test_correlation_helper_on_derivative_order_over_full_window<0>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order0_mid_window_Test)
{
    test_correlation_helper_on_derivative_order_over_mid_window<0>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order1_full_window_Test)
{
    test_correlation_helper_on_derivative_order_over_full_window<1>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order1_mid_window_Test)
{
    test_correlation_helper_on_derivative_order_over_mid_window<1>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order2_full_window_Test)
{
    test_correlation_helper_on_derivative_order_over_full_window<2>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order2_mid_window_Test)
{
    test_correlation_helper_on_derivative_order_over_mid_window<2>();
}
// wild overkill, but good to stress test the code:
TEST(FFTTest, CorrelationHelperViaFFT_order3_full_window_Test)
{
    test_correlation_helper_on_derivative_order_over_full_window<3>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order3_mid_window_Test)
{
    test_correlation_helper_on_derivative_order_over_mid_window<3>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order4_full_window_Test)
{
    test_correlation_helper_on_derivative_order_over_full_window<4>();
}
TEST(FFTTest, CorrelationHelperViaFFT_order4_mid_window_Test)
{
    test_correlation_helper_on_derivative_order_over_mid_window<4>();
}



TEST(ReconstructionTest, ReconstructionTestManual)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    types::array_p<domain::WindowSize> reconstructed;
    for (size_t k = 0; k < reconstructed.size(); ++k)
    {
        reconstructed[k] = chirp.manually_reconstruct_at_index(k);
        reconstructed[k] /= domain::WindowSize; // our transforms are unnormalized
    }

    assert_eq(reconstructed, chirp.input);
}

TEST(ReconstructionTest, ReconstructionTestFFT)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    types::array_p<domain::WindowSize> reconstructed;
    utils::FFT_real_1d<domain::WindowSize> fft_real_1d{};

    fft_real_1d.reset(+1);
    fft_real_1d.c2r(chirp.coeffs, reconstructed);
    fft_real_1d.rescale(reconstructed);

    assert_eq(reconstructed, chirp.input);
}


template <typename CorrelationHelperT, typename FFTHelperT, typename TauT>
auto eval_and_print(const CorrelationHelperT & correlation_helper, const FFTHelperT & signal, const TauT tau)
{
    const auto &B = signal.coeffs;
    types::Precision duration_s = domain::window_period_s;
    constexpr size_t Nsamples = domain::WindowSize;

    auto [f, f_d1, f_d2] = correlation_helper.template correlate_and_derive<2>(B, tau);

    auto sample_period_s = duration_s / Nsamples;

    {
        std::stringstream ss;
        ss << std::fixed << std::showpoint << std::showpos;
        ss << std::setprecision(6);
        ss << "tau_index:" << tau / sample_period_s;
        ss << std::setprecision(8);
        ss << " (" << tau << "s)";
        std::cout << ss.str();
    }

    {
        std::stringstream ss;
        ss << std::scientific << std::showpos;
        ss << std::setprecision(8);
        ss << ". f: " << f << ", f_d1: " << f_d1 << ", f_d2:" << f_d2 << "\n";
        std::cout << ss.str();
    }
};

void peak_finder_test(types::Precision offset_s)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    // auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = utils::get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::CorrelationHelper<domain::WindowSize, 2> correlation_helper;
    correlation_helper.setup(chirp.coeffs, domain::cd_freq_hz);



    auto fd0_fd1_fd2 = [&correlation_helper, &signal](auto tau)
    { 
        constexpr size_t derivative_order = 2;
        return correlation_helper.correlate_and_derive<derivative_order>(signal.coeffs, tau);
    };

    static constexpr bool verbose = true;

    auto guessed_tau_s = tau_true_s + (offset_s < 0 ? -1 : +1) * 0.5 * domain::sample_period_s;
    auto [optimal_tau_s, optimal_value] = utils::newton(fd0_fd1_fd2, guessed_tau_s);

    if constexpr (verbose)
    {
        std::cout << "starting: ";
        eval_and_print(correlation_helper, signal, guessed_tau_s);
        std::cout << "solution: ";
        eval_and_print(correlation_helper, signal, optimal_tau_s);
        std::cout << "tau_true: ";
        eval_and_print(correlation_helper, signal, tau_true_s);
    }

    auto [f, f_d1, f_d2] = fd0_fd1_fd2(optimal_tau_s);
    ASSERT_LT(std::abs(f_d1), 1E-10) << "derivative should be super close to 0";

    auto residual_s = (tau_true_s - optimal_tau_s);
    auto residual_mm = constants::speed_of_sound_mmps * residual_s;
    std::cout << "residual: " << residual_s/domain::sample_period_s << "(fractions of a sample), " << residual_mm << "mm\n";

    ASSERT_LT(std::abs(residual_s), residual_s_tolerance);
    ASSERT_LT(std::abs(residual_mm), residual_mm_tolerance_tight);
}


TEST(PeakFinderTest, PeakFinderTest_PositiveTau)
{
    types::Precision offset_s = 0.52 * domain::sample_period_s;
    peak_finder_test(offset_s);
}

TEST(PeakFinderTest, PeakFinderTest_NegativeTau)
{
    types::Precision offset_s =  -0.52 * domain::sample_period_s;
    peak_finder_test(offset_s);
}

TEST(ExtremmaFinderTest, ExtremmaFinderTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = utils::get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto time_true_s = signal_params.center_s;

    utils::CorrelationHelper<domain::WindowSize, 2> correlation_helper;
    // correlation_helper.setup(chirp.coeffs, domain::cd_freq_hz);
    correlation_helper.setup(chirp.coeffs, 1); // LOOK HERE. ASSUMING ONE SAMPLE A SECOND SO WE CAN INDEX

    auto correlate_and_derive = [&correlation_helper, &signal]<size_t derivative_order>(auto tau)
    {
        return correlation_helper.correlate_and_derive<derivative_order>(signal.coeffs, tau);
    };

    correlation_helper.set_correlation_surface_via_fft<0>(signal.coeffs);
    correlation_helper.set_correlation_surface_via_fft<1>(signal.coeffs);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    uint64_t tau_to_time_offset = domain::WindowSize/2; // tau of 0 means a peak at this time
    utils::ExtremmaFinder extremma_helper;
    extremma_helper.reset(n_extremma);
    extremma_helper.find_extremma(correlation_helper, signal.coeffs, tau_to_time_offset);

    // std::cout << "min heap:\n";
    // std::cout << "time_true_s; " << time_true_s << "\n"; 
    // extremma_helper.print(domain::sample_period_s, tau_to_time_offset);

    auto & strongest_peak = extremma_helper.extremma()[0];
    auto found_peak_time_s = strongest_peak.to_time_s(domain::cd_freq_hz);
    auto residual_s = found_peak_time_s - time_true_s;
    auto residual_mm = residual_s * constants::speed_of_sound_mmps;

    // std::cout << "XXX\n";
    // eval_and_print(correlation_helper, signal, found_peak_time_s);

    ASSERT_LT(std::abs(residual_s), residual_s_tolerance);
    ASSERT_LT(std::abs(residual_mm), residual_mm_tolerance_tight);
}

TEST(IngestorTest, IngestorSimpleTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = utils::get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto time_true_s = signal_params.center_s;

    size_t n_extremma = 2 * 3 + 1;

    std::cout << "run ingestor on sinc-sinc data\n";
    {
        auto chirp_input = chirp.input;

        typedef utils::Ingestor<domain::WindowSize> IngestorT;
        IngestorT ingestor;
        ingestor.reset(chirp_input);
        constexpr bool reset_heap = true;
        IngestorT::types::index_type tau_to_time_offset_idx{};
        size_t block_index{};
        auto signal_ptr = signal.input.data();
        // std::cout << "sinc_insertion_time_s: " << sinc_insertion_time_s << "\n";
    
        constexpr size_t channel_index = 0;

        tau_to_time_offset_idx = block_index * domain::BlockSize; // 0
        signal_ptr = signal.input.data() + block_index * domain::BlockSize;
        std::cout << "found peaks in first half (dirty start):\n";
        std::cout << "added " << ingestor.run(channel_index, signal_ptr, tau_to_time_offset_idx, n_extremma, reset_heap) << " extremma\n";
        ingestor.extremma_helper().print(std::cout, domain::sample_period_s, tau_to_time_offset_idx);
        ++block_index;

        tau_to_time_offset_idx = block_index * domain::BlockSize; // tau of 0 means peak at this time (WindowSize/2)
        signal_ptr = signal.input.data() + block_index * domain::BlockSize;
        std::cout << "found peaks whole window (now warmed up):\n";
        std::cout << "added " << ingestor.run(channel_index, signal_ptr, tau_to_time_offset_idx, n_extremma, reset_heap) << " extremma\n";
        ingestor.extremma_helper().print(std::cout, domain::sample_period_s, tau_to_time_offset_idx);
        ++block_index;


        auto & strongest_peak = ingestor.extremma_helper().extremma()[0];
        auto found_peak_time_s = strongest_peak.to_time_s(domain::cd_freq_hz);
        auto residual_s = found_peak_time_s - time_true_s;
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;

        ASSERT_LT(std::abs(residual_s), residual_s_tolerance);
        ASSERT_LT(std::abs(residual_mm), residual_mm_tolerance_tight);
    }
}

template <bool ChirpsOnly=false, typename SoundFunctionT>
static auto generate_random_sound_for_domain(const /*types::*/SoundFunctionT &chirp_func, const utils::WaveParams &chirp_params, size_t n_chirps, types::Precision sync_period_s=1.0)
{
    std::vector<utils::WaveParams> sound_params;
    types::Precision sound_amplitude = 0.5 * chirp_params.amplitude;
    if (!ChirpsOnly)
    {
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.0 * domain::window_period_s, .freq_hz = types::Precision(2*5.0 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.3 * domain::window_period_s, .freq_hz = types::Precision(2*4.0 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.4 * domain::window_period_s, .freq_hz = types::Precision(2*3.3 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.5 * domain::window_period_s, .freq_hz = types::Precision(2*2.3 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.6 * domain::window_period_s, .freq_hz = types::Precision(2*1.3 * chirp_params.freq_hz)});
    }
    std::vector<std::pair<utils::WaveParams, /*types::*/SoundFunctionT>> multi_chirp_params;
    for (size_t i = 0; i < n_chirps; ++i)
    {
        types::Precision chirp_center_s = i*sync_period_s + 0.5;
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
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;

    std::cout << "\n\n";
    std::cout << "run ingestor on long running, generic audio\n";

    constexpr size_t n_chirps = 3;
    constexpr bool chirps_only = true;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps);

    auto chirp_input = chirp.input;

    typedef utils::Ingestor<domain::WindowSize> IngestorT;
    IngestorT ingestor;
    ingestor.reset(chirp_input);
    ingestor.extremma_helper().reset(n_extremma);
    constexpr size_t channel_index = 0;

    auto n_blocks = n_samples / domain::BlockSize; // signal_inputs.size() / domain::BlockSize;
    for (size_t block_index = 0; block_index < n_blocks; ++block_index)
    {
        constexpr bool reset_heap = false;
        IngestorT::types::index_type tau_to_time_offset_idx = block_index * domain::BlockSize;
        auto signal_block = get_next_block();
        auto signal_ptr = signal_block.data();
        auto n_extremma_added = ingestor.run(channel_index, signal_ptr, tau_to_time_offset_idx, n_extremma, reset_heap);

        // optional print
        if (0 == block_index || block_index == n_blocks-1)
        {
            std::cout << "block_index = " << block_index << "[], tau_to_time_offset_idx = " << 0*tau_to_time_offset_idx << "[]\n";
            ingestor.extremma_helper().print(std::cout, domain::sample_period_s, 0*tau_to_time_offset_idx);
            std::cout << "";
        }
    }

    std::cout << "residuals for the N largest peaks\n";
    utils::Ingestor<domain::WindowSize>::vector_extremma extremma_sorted = ingestor.extremma_helper().extremma(); // deep copy
    extremma_sorted.resize(n_chirps);
    std::sort(extremma_sorted.begin(), extremma_sorted.end(), [](const IngestorT::ExtremmaFinderT::CandidateExtremma & ex1, const IngestorT::ExtremmaFinderT::CandidateExtremma & ex2){return ex1.time_idx < ex2.time_idx;});
    for (size_t i = 0; i < n_chirps; ++i)
    {
        auto residual_s = multi_chirp_params[i].first.center_s - extremma_sorted[i].to_time_s(domain::cd_freq_hz);
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;
        auto residual_percentsamplerate = residual_s / domain::sample_period_s * 100.0;
        std::cout << "chirp["<<i<<"]: time_idx: "<<multi_chirp_params[i].first.center_s/domain::sample_period_s<<", residual_s: " << residual_s << "s, residual_mm: " << residual_mm << "mm, residual_%samplerate: " <<residual_percentsamplerate<< "%\n";

        EXPECT_LT(std::abs(residual_s), residual_s_tolerance);
        EXPECT_LT(std::abs(residual_mm), residual_mm_tolerance_tight);
    }
    
}

TEST(SignalLockTest, SignalLockTest_LF_NoEnvSound)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    types::Precision sync_period_s = 1.0; // 1s worth of samples
    types::Precision sync_half_gate_s = 1.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5;
    types::Precision nearby_peak_toleranc_s = 1.0 / chirp_params.freq_hz * 1.5;

    size_t sync_period_idx = sync_period_s * domain::cd_freq_hz;
    size_t sync_half_gate_idx = sync_half_gate_s * domain::cd_freq_hz;
    size_t nearby_peak_tolerance_idx = nearby_peak_toleranc_s * domain::cd_freq_hz;

    constexpr size_t n_chirps = 10;
    constexpr bool chirps_only = true;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps, sync_period_s);

    auto chirp_input = chirp.input;

    typedef utils::SignalAcquirer<domain::WindowSize> AcquirerT;
    AcquirerT acquirer;
    acquirer.reset(chirp_input);
    acquirer.extremma_helper().reset(n_extremma);
    constexpr size_t channel_index = 0;

    std::vector<AcquirerT::ExtremmaFinderT::CandidateExtremma> detected;
    auto on_chirp = [&](size_t channel_index, const AcquirerT::ExtremmaFinderT::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " channel_index: "<< channel_index<< " time_idx: " << det.time_idx << " value: " << det.value<< "\n";
        std::cout << ss.str();
        detected.push_back(det);
    };

    auto n_blocks = n_samples / domain::BlockSize;// signal_inputs.size() / domain::BlockSize;
    for (size_t block_index = 0; block_index < n_blocks; ++block_index)
    {
        AcquirerT::types::index_type tau_to_time_offset_idx = block_index * domain::BlockSize;
        auto signal_block = get_next_block();
        auto signal_ptr = signal_block.data();
        acquirer.run(channel_index, signal_ptr, on_chirp, tau_to_time_offset_idx, n_extremma, sync_period_idx, sync_half_gate_idx, nearby_peak_tolerance_idx);

        // optional print
        if (/*0 == block_index ||*/ block_index == n_blocks-1)
        {
            std::cout << "block_index = " << block_index << "[], tau_to_time_offset_idx = " << 0*tau_to_time_offset_idx << "[]\n";
            acquirer.extremma_helper().print(std::cout, domain::sample_period_s, 0*tau_to_time_offset_idx);
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
        auto residual_s = params_this_chirp.first.center_s - detected[i].to_time_s(domain::cd_freq_hz);
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;
        auto residual_percentsamplerate = residual_s / domain::sample_period_s * 100.0;
        std::cout << "chirp["<<i<<"]: residual_s: " << residual_s << "s, residual_mm: " << residual_mm << "mm, residual_%samplerate: " <<residual_percentsamplerate<< "%\n";

        EXPECT_LT(std::abs(residual_s), residual_s_tolerance);
        EXPECT_LT(std::abs(residual_mm), residual_mm_tolerance_tight);
    }

}


TEST(SignalLockTest, PeakDetector)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 15E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    types::Precision sync_period_s = 1.0;
    types::Precision sync_half_gate_s = 50.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5;
    types::Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;

    size_t sync_period_idx = sync_period_s * domain::cd_freq_hz;
    size_t sync_half_gate_idx = sync_half_gate_s * domain::cd_freq_hz;
    size_t nearby_peak_tolerance_idx = nearby_peak_tolerance_s * domain::cd_freq_hz;

    constexpr size_t n_chirps = 300;
    constexpr bool chirps_only = false;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps, sync_period_s);

    auto chirp_input = chirp.input;

    typedef utils::SignalAcquirer<domain::WindowSize> AcquirerT;
    AcquirerT acquirer;
    acquirer.reset(chirp_input);
    acquirer.extremma_helper().reset(n_extremma);
    constexpr size_t channel_index = 0;

    std::vector<AcquirerT::ExtremmaFinderT::CandidateExtremma> detected;
    auto on_chirp = [&](size_t channel_index, const AcquirerT::ExtremmaFinderT::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " channel_index: "<< channel_index<< " time_idx: " << det.time_idx << " value: " << det.value<< "\n";
        std::cout << ss.str();
        detected.push_back(det);
    };

    AcquirerT::vector_extremma extremma;
    extremma.push_back({.time_idx= static_cast<AcquirerT::types::index_type>(+31.09244246*domain::cd_freq_hz), .value=-9.32685364e+06});
    extremma.push_back({.time_idx= static_cast<AcquirerT::types::index_type>(+33.09226129*domain::cd_freq_hz), .value=-9.40550662e+06});
    extremma.push_back({.time_idx= static_cast<AcquirerT::types::index_type>(+32.09090072*domain::cd_freq_hz), .value=-9.15504890e+06});
    extremma.push_back(AcquirerT::ExtremmaFinderT::CandidateExtremma());
    extremma.push_back(AcquirerT::ExtremmaFinderT::CandidateExtremma());
    extremma.push_back(AcquirerT::ExtremmaFinderT::CandidateExtremma());
    extremma.push_back(AcquirerT::ExtremmaFinderT::CandidateExtremma());

    types::Precision tau_to_time_offset_idx = 11400 * domain::BlockSize;
    acquirer.fire_on_new_peak(channel_index, extremma, on_chirp, tau_to_time_offset_idx, sync_period_idx, sync_half_gate_idx);

    auto n_detected_chirps = detected.size();
    ASSERT_EQ(detected.size(), 1) << "didnt detect the right number of chirps (1)";
}


TEST(SignalLockTest, SignalLockTest_HF)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 15E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    types::Precision sync_period_s = 1.0;
    types::Precision sync_half_gate_s = 1.0 * constants::in_to_mm / constants::speed_of_sound_mmps * 0.5;
    types::Precision nearby_peak_tolerance_s = 1.0 / chirp_params.freq_hz * 1.5;

    size_t sync_period_idx = sync_period_s * domain::cd_freq_hz;
    size_t sync_half_gate_idx = sync_half_gate_s * domain::cd_freq_hz;
    size_t nearby_peak_tolerance_idx = nearby_peak_tolerance_s * domain::cd_freq_hz;

    constexpr size_t n_chirps = 60;
    constexpr bool chirps_only = false;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps, sync_period_s);

    auto chirp_input = chirp.input;

    typedef utils::SignalAcquirer<domain::WindowSize> AcquirerT;
    AcquirerT acquirer;
    acquirer.reset(chirp_input);
    acquirer.extremma_helper().reset(n_extremma);
    constexpr size_t channel_index = 0;

    std::vector<AcquirerT::ExtremmaFinderT::CandidateExtremma> detected;
    auto on_chirp = [&](size_t channel_index, const AcquirerT::ExtremmaFinderT::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " channel_index: "<< channel_index<< " time_idx: " << det.time_idx << " value: " << det.value<< "\n";
        std::cout << ss.str();
        detected.push_back(det);
    };

    auto n_blocks = n_samples / domain::BlockSize;
    for (size_t block_index = 0; block_index < n_blocks; ++block_index)
    {
        auto tau_to_time_offset_idx = block_index * domain::BlockSize;
        auto signal_block = get_next_block();
        auto signal_ptr = signal_block.data();
        acquirer.run(channel_index, signal_ptr, on_chirp, tau_to_time_offset_idx, n_extremma, sync_period_idx, sync_half_gate_idx, nearby_peak_tolerance_idx);


        // optional print
        if (/*0 == block_index ||*/ block_index == n_blocks-1)
        {
            std::cout << "block_index = " << block_index << "[], tau_to_time_offset_idx = " << 0*tau_to_time_offset_idx << "[]\n";
            acquirer.extremma_helper().print(std::cout, domain::sample_period_s, 0*tau_to_time_offset_idx);
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
        auto residual_s = params_this_chirp.first.center_s - detected[i].to_time_s(domain::cd_freq_hz);
        auto residual_mm = residual_s * constants::speed_of_sound_mmps;
        auto residual_percentsamplerate = residual_s / domain::sample_period_s * 100.0;
        std::cout << "chirp["<<i<<"]: residual_s: " << residual_s << "s, residual_mm: " << residual_mm << "mm, residual_%samplerate: " <<residual_percentsamplerate<< "%\n";

        EXPECT_LT(std::abs(residual_s), residual_s_tolerance);
        EXPECT_LT(std::abs(residual_mm), residual_mm_tolerance_loose);
    }
}
