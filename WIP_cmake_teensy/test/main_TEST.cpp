
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

static utils::WaveParams get_offset_params(types::Precision sample_period_s, const utils::WaveParams &chirp_params, types::Precision offset_s)
{
    types::Precision sinc_insertion_time_s = chirp_params.center_s + offset_s;
    return utils::WaveParams{.amplitude = chirp_params.amplitude, .center_s = sinc_insertion_time_s, .freq_hz = chirp_params.freq_hz};
}

template <typename T>
void assert_eq(const T& arrA, const T& arrB, double tol=1E-10)
{
        types::Precision total_error = 0;
        for (size_t i = 0; i < arrA.size(); ++i)
        {
            auto this_error = std::abs(arrA[i] - arrB[i]);
            // std::cout << arrA[i] / arrB[i] << "\n";
            total_error += this_error * this_error;
        }
        total_error = std::sqrt(total_error);
        std::cout << "total error: " << total_error << "\n";
        std::cout << "average error: " << total_error/arrA.size() << "\n";
        ASSERT_LT(total_error, tol);
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


    if (true)
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

    std::cout << "jac size: (" << jac_Nrows << ", " << jac_Ncols << ")\n";
    std::cout << "jac:\n" << jac << "\n";


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
        std::cout << "meas_est: " << measurements_est.transpose() << "\n";
        std::cout << "states_est: " << states_est.transpose() << "\n";
        std::cout << "update: (norm=" << update.norm() << "), " << update.transpose() << "\n";

        Eigen::FullPivLU<Jac> lu_decomp(jac);
        auto rank = lu_decomp.rank();
        std::cout << "jac size: (" << jac_Nrows << ", " << jac_Ncols << "), rank: "<< rank << "\n";

        std::cout << "\n";
        states_est -= update;

        // if (update.norm() < 0.001)
        // {
        //     break;
        // }
    }

    States states_residual = states_true - states_est;
    std::cout << "residual: (norm=" << states_residual.norm() << "), " << states_residual.transpose() << "\n";
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

    utils::DFT_c2c_1d<Nsamples> forward;
    forward.reset();

    // run new method
    typename utils::DFT_c2c_1d<Nsamples>::OutputT coeffs;
    coeffs.fill(0);
    forward.run(input.data(), coeffs);


    utils::DFT_c2c_1d<Nsamples> inverse;
    inverse.reset(+1.0);

    // run new method
    typename utils::DFT_c2c_1d<Nsamples>::OutputT reconstruct;
    reconstruct.fill(0);
    inverse.run(coeffs.data(), reconstruct);
    inverse.rescale(reconstruct);

    ASSERT_EQ(coeffs.size(), input.size());
    ASSERT_EQ(coeffs.size(), reconstruct.size());

    assert_eq(input, reconstruct);
}

#define TEST_DFT_C2C(NAME, N)              \
TEST(FFTTest, DFT_C2C_##NAME)              \
{                                          \
    run_c2c_test_for_Nsamples<N>();        \
}

TEST_DFT_C2C(N1, 1)
TEST_DFT_C2C(N2, 2)
TEST_DFT_C2C(N4, 4)
TEST_DFT_C2C(N8, 8)
TEST_DFT_C2C(N16, 16)
TEST_DFT_C2C(N32, 32)
TEST_DFT_C2C(N64, 64)
TEST_DFT_C2C(N128, 128)
TEST_DFT_C2C(N256, 256)
TEST_DFT_C2C(BlockSize, domain::BlockSize)
TEST_DFT_C2C(WindowSize, domain::WindowSize)

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

    utils::DFT_real_1d<Nsamples> dft_forward;

    // run new method
    typename utils::DFT_real_1d<Nsamples>::CoeffsT manual_coeffs;
    manual_coeffs.fill(0);
    dft_forward.r2c(input, manual_coeffs);

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

        utils::DFT_real_1d<Nsamples> dft_inverse(+1);
        dft_inverse.c2r(manual_coeffs, reconstructed);
        dft_inverse.rescale(reconstructed);

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

#define TEST_DFT_R2C(NAME, N)              \
TEST(FFTTest, DFT_R2C_##NAME)              \
{                                          \
    run_r2c_test_for_Nsamples<N>();        \
}

TEST_DFT_R2C(N1, 1)
TEST_DFT_R2C(N2, 2)
TEST_DFT_R2C(N4, 4)
TEST_DFT_R2C(N8, 8)
TEST_DFT_R2C(N16, 16)
TEST_DFT_R2C(N32, 32)
TEST_DFT_R2C(N64, 64)
TEST_DFT_R2C(N128, 128)
TEST_DFT_R2C(N256, 256)
TEST_DFT_R2C(BlockSize, domain::BlockSize)
TEST_DFT_R2C(WindowSize, domain::WindowSize)


TEST(FFTTest, DerivativeHelperViaDFT_Test)
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
    signal.transform_and_normalize_coefficients();


    utils::DerivativeHelper<domain::WindowSize, 2> derivative_helper;
    derivative_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    // derivative_helper.correlate_via_dft<0>(signal.coeffs);
    // dft.rescale(butterfly_results);


    auto test_dft_results_against_manual_implementation_whole_window = [&]<size_t derivative_order>()
    {
        derivative_helper.correlate_via_dft<derivative_order>(signal.coeffs);

        types::Precision tau_s_lower_bound = 0.0; // -1.0 /* whole window */ * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_upper_bound = +2.0 /* whole window */ * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_step = domain::sample_period_s;

        // the chirp was centered at half the window duration, so the 0th index in the fft outputs
        // should correspond to tau of 0
        types::array_p<domain::WindowSize> nsquared_results;
        auto tau_s = tau_s_lower_bound;
        for (size_t i = 0; i < nsquared_results.size(); ++i)
        {
            nsquared_results[i] = derivative_helper.correlate_and_derive<derivative_order>(signal.coeffs, tau_s)[derivative_order];
            tau_s += tau_s_step;
        }

        assert_eq(derivative_helper.correlation_surface[derivative_order], nsquared_results, 1E-8);
    };

    auto test_dft_results_against_manual_implementation_mid_window = [&]<size_t derivative_order>()
    {
        derivative_helper.correlate_via_dft<derivative_order>(signal.coeffs);

        types::Precision tau_s_lower_bound = -0.5 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_upper_bound = +0.5 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
        types::Precision tau_s_step = domain::sample_period_s;

        types::array_p<domain::BlockSize> nsquared_results;
        auto tau_s = tau_s_lower_bound;
        for (size_t i = 0; i < nsquared_results.size(); ++i)
        {
            nsquared_results[i] = derivative_helper.correlate_and_derive<derivative_order>(signal.coeffs, tau_s)[derivative_order];
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
            fft_results_reordered[i++] = derivative_helper.correlation_surface[derivative_order][offset+i];
        }

        // this is the "positive tau" side of the results. all the way from 0 to most positive
        for (size_t j = 0 ; j < domain::BlockSize/2; ++j)
        {
            fft_results_reordered[i++] = derivative_helper.correlation_surface[derivative_order][j];
        }

        assert_eq(fft_results_reordered, nsquared_results, 1E-8);
    };



    // dft.rescale(butterfly_results);

    test_dft_results_against_manual_implementation_whole_window.template operator()<0>();
    test_dft_results_against_manual_implementation_mid_window.template operator()<0>();
    test_dft_results_against_manual_implementation_whole_window.template operator()<1>();
    test_dft_results_against_manual_implementation_mid_window.template operator()<1>();
    // test_dft_results_against_manual_implementation_whole_window.template operator()<2>();
    // test_dft_results_against_manual_implementation_mid_window.template operator()<2>();

}

// TEST(ReconstructionTest, ReconstructionTest)
// {
//     // chirp
//     auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
//     auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
//     auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

//     double largest_error = std::numeric_limits<double>::lowest();
//     for (size_t k = 0; k < domain::WindowSize; ++k)
//     {
//         auto reconstructed = utils::reconstruct_at_index(chirp.coeffs, k);
//         double this_error = std::abs(reconstructed - chirp.input[k]);
//         largest_error = std::max(largest_error, this_error);
//     }
//     // std::cout << "largest reconstruction error: " << (largest_error) << "\n";

//     ASSERT_LT(largest_error, 1E-4);
// }


TEST(DerivativeHelperTest, DerivativeHelperTest)
{
    // chirp
    auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
    auto chirp_params = utils::WaveParams{.amplitude = 1.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 1E3};
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::DerivativeHelper<domain::WindowSize, 2> derivative_helper;
    derivative_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    auto correlate_and_derive = [&]<size_t derivative_order>(auto tau)
    {
        return derivative_helper.correlate_and_derive<derivative_order>(signal.coeffs, tau);
    };

    auto fd0_fd1 = [&](auto tau)
    { return correlate_and_derive.template operator()<1>(tau); };
    auto fd0_fd1_fd2 = [&](auto tau)
    { return correlate_and_derive.template operator()<2>(tau); };

    // auto eval_and_print = [&](auto tau)
    // {
    //     derivative_helper.eval_and_print(chirp.coeffs, signal.coeffs, domain::window_period_s, tau);
    // };

    auto guessed_tau_s = 0 + 0.5 * domain::sample_period_s;
    std::cout << "starting: ";
    // eval_and_print(guessed_tau_s);
    std::cout << "solution: ";
    auto optimal_tau_s = std::get<0>(utils::newton(fd0_fd1_fd2, guessed_tau_s));
    // eval_and_print(optimal_tau_s);
    std::cout << "tau_true: ";
    // eval_and_print(tau_true_s);

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
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

    // signal
    auto offset_s = 0.52 * domain::sample_period_s;
    auto signal_params = get_offset_params(domain::sample_period_s, chirp_params, offset_s);
    auto signal = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, signal_params);
    auto tau_true_s = offset_s;

    utils::DerivativeHelper<domain::WindowSize, 2> derivative_helper;
    derivative_helper.setup(chirp.coeffs, domain::cd_freq_hz);

    auto correlate_and_derive = [&]<size_t derivative_order>(auto tau)
    {
        return derivative_helper.correlate_and_derive<derivative_order>(signal.coeffs, tau);
    };

    // auto fd0 = [&](auto tau)
    // { return correlate_and_derive.template operator()<0>(tau)[0]; };
    // auto fd1 = [&](auto tau)
    // { return correlate_and_derive.template operator()<1>(tau)[1]; };
    // auto fd0_fd1_fd2 = [&](auto tau)
    // { return correlate_and_derive.template operator()<2>(tau); };


    derivative_helper.correlate_via_dft<0>(signal.coeffs);
    derivative_helper.correlate_via_dft<1>(signal.coeffs);

    // implement a search
    size_t n_extremma = 2 * 3 + 1;
    auto tau_s_lower_bound = -0.5 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
    auto tau_s_upper_bound = +0.5 * static_cast<int>(domain::BlockSize) * domain::sample_period_s;
    auto tau_s_step = domain::sample_period_s;
    utils::ExtremmaFinder extremma_helper;
    extremma_helper.reset(n_extremma);
    extremma_helper.find_extremma(derivative_helper, signal.coeffs, tau_s_lower_bound, tau_s_upper_bound, tau_s_step);

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
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

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
    
        constexpr size_t channel_index = 0;

        tau_to_time_offset_s = block_index*domain::block_period_s;
        signal_ptr = signal.input.data() + block_index * domain::BlockSize;
        std::cout << "found peaks in first half (dirty start):\n";
        std::cout << "added " << ingestor.run(channel_index, signal_ptr, tau_to_time_offset_s, n_extremma, reset_heap) << " extremma\n";
        ingestor.extremma_helper().print(domain::sample_period_s);
        ++block_index;

        tau_to_time_offset_s = block_index*domain::block_period_s;
        signal_ptr = signal.input.data() + block_index * domain::BlockSize;
        std::cout << "found peaks whole window (now warmed up):\n";
        std::cout << "added " << ingestor.run(channel_index, signal_ptr, tau_to_time_offset_s, n_extremma, reset_heap) << " extremma\n";
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
    types::Precision sound_amplitude = 0.5 * chirp_params.amplitude;
    if (!ChirpsOnly)
    {
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.0 * domain::window_period_s, .freq_hz = types::Precision(2*5.0 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.3 * domain::window_period_s, .freq_hz = types::Precision(2*4.0 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.4 * domain::window_period_s, .freq_hz = types::Precision(2*3.3 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.5 * domain::window_period_s, .freq_hz = types::Precision(2*2.3 * chirp_params.freq_hz)});
        sound_params.push_back(utils::WaveParams{.amplitude = sound_amplitude, .center_s = 0.6 * domain::window_period_s, .freq_hz = types::Precision(2*1.3 * chirp_params.freq_hz)});
    }
    std::vector<std::pair<utils::WaveParams, types::SoundFunctionT>> multi_chirp_params;
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

    utils::Ingestor<domain::WindowSize> ingestor;
    ingestor.reset(chirp_input, domain::cd_freq_hz);
    ingestor.extremma_helper().reset(n_extremma);
    constexpr size_t channel_index = 0;

    auto n_blocks = n_samples / domain::BlockSize; // signal_inputs.size() / domain::BlockSize;
    for (size_t block_index = 0; block_index < n_blocks; ++block_index)
    {
        size_t sample_index = block_index * domain::BlockSize;
        auto block_time_s = block_index * domain::block_period_s;
        constexpr bool reset_heap = false;
        auto tau_to_time_offset_s = block_time_s;
        auto signal_block = get_next_block();
        auto signal_ptr = signal_block.data();// signal_inputs.data() + block_index * domain::BlockSize;
        auto n_extremma_added = ingestor.run(channel_index, signal_ptr, tau_to_time_offset_s, n_extremma, reset_heap);

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
    auto chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

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
    constexpr size_t channel_index = 0;

    std::vector<utils::ExtremmaFinder::CandidateExtremma> detected;
    auto on_chirp = [&](size_t channel_index, const utils::ExtremmaFinder::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " channel_index: "<< channel_index<< " time_s: " << det.time_s << " value: " << det.value<< /*" id: " << det.id << */"\n";
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
        acquirer.run(channel_index, signal_ptr, on_chirp, tau_to_time_offset_s, n_extremma, sync_period_s, sync_half_gate_s, nearby_peak_tolerance_s);


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

    constexpr size_t n_chirps = 300;
    constexpr bool chirps_only = false;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps, sync_period_s);

    auto chirp_input = chirp.input;

    utils::SignalAcquirer<domain::WindowSize> acquirer;
    acquirer.reset(chirp_input, domain::cd_freq_hz);
    acquirer.extremma_helper().reset(n_extremma);
    constexpr size_t channel_index = 0;

    std::vector<utils::ExtremmaFinder::CandidateExtremma> detected;
    auto on_chirp = [&](size_t channel_index, const utils::ExtremmaFinder::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " channel_index: "<< channel_index<< " time_s: " << det.time_s << " value: " << det.value<< " id: " << /*det.id <<*/ "\n";
        std::cout << ss.str();
        detected.push_back(det);
    };

    utils::SignalAcquirer<domain::WindowSize>::vector_extremma extremma;
    extremma.push_back({.time_s= +31.09244246, .value=-9.32685364e+06});
    extremma.push_back({.time_s= +33.09226129, .value=-9.40550662e+06});
    extremma.push_back({.time_s= +32.09090072, .value=-9.15504890e+06});
    extremma.push_back(utils::ExtremmaFinder::CandidateExtremma());
    extremma.push_back(utils::ExtremmaFinder::CandidateExtremma());
    extremma.push_back(utils::ExtremmaFinder::CandidateExtremma());
    extremma.push_back(utils::ExtremmaFinder::CandidateExtremma());

    types::Precision tau_to_time_offset_s = 11400 * domain::block_period_s;
    acquirer.fire_on_new_peak(channel_index, extremma, on_chirp, tau_to_time_offset_s, sync_period_s, sync_half_gate_s);

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

    constexpr size_t n_chirps = 60;
    constexpr bool chirps_only = false;
    auto [n_samples, get_next_block, multi_chirp_params] = generate_random_sound_for_domain<chirps_only>(chirp_func, chirp_params, n_chirps, sync_period_s);

    auto chirp_input = chirp.input;

    utils::SignalAcquirer<domain::WindowSize> acquirer;
    acquirer.reset(chirp_input, domain::cd_freq_hz);
    acquirer.extremma_helper().reset(n_extremma);
    constexpr size_t channel_index = 0;

    std::vector<utils::ExtremmaFinder::CandidateExtremma> detected;
    auto on_chirp = [&](size_t channel_index, const utils::ExtremmaFinder::CandidateExtremma & det)
    {
        std::stringstream ss;
        ss <<  "chirp detected:" << " channel_index: "<< channel_index<< " time_s: " << det.time_s << " value: " << det.value<< " id: " << /*det.id <<*/ "\n";
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
        acquirer.run(channel_index, signal_ptr, on_chirp, tau_to_time_offset_s, n_extremma, sync_period_s, sync_half_gate_s, nearby_peak_tolerance_s);


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
