
#include "utils.hpp"
#include "domain.hpp"
#include <complex>

// fully naive implementation. 
// compute std::exp live (no precompute)
// multiply all coefficients, N*N operations
template <typename TimeFuncT>
auto naive_c2c_live_exp(const TimeFuncT & time_func)
{
    constexpr size_t Nsamples = domain::WindowSize;
    constexpr size_t Ncoeffs = Nsamples;

    types::array_cp<Nsamples> input_a;
    types::array_cp<Nsamples> input_b;
    for (size_t i = 0; i < input_a.size(); ++i)
    {
        input_a[i] = types::cPrecision((i*i) % 17, 0.0);
        input_b[i] = types::cPrecision((i*i*i) % 23, 0.0);
    }

    types::array_cp<Ncoeffs> coeffs_a;
    types::array_cp<Ncoeffs> coeffs_b;

    utils::DFT_c2c_1d<Nsamples> transformer{};
    transformer.reset(-1.0);
    transformer.run(input_a.data(), coeffs_a);
    transformer.run(input_b.data(), coeffs_b);

    types::array_p<Nsamples> surface;

    auto time_start = time_func();

    for (size_t output_index = 0; output_index < Nsamples; ++output_index)
    {
        types::Precision sum(0.0);
        for (size_t i = 0; i < coeffs_a.size(); ++i)
        {
            auto a_conj = std::conj(coeffs_a[i]);
            auto b = coeffs_b[i];

            types::Precision freq = output_index * i / types::Precision(domain::WindowSize);

            types::cPrecision term_i = a_conj * b * std::exp(constants::twopij * freq);

            sum += term_i.real();
        }

        surface[output_index] = sum;
    }

    auto time_stop = time_func();
    return time_stop - time_start;
}

// still naive implementation
// compute std::live  (no precompute)
// use half the coefficients by leveraging conjugate symmetry
// use half the outputs since we dont need them
template <typename TimeFuncT>
auto naive_r2c_live_exp(const TimeFuncT & time_func)
{
    constexpr size_t Nsamples = domain::WindowSize;
    constexpr size_t Ncoeffs = Nsamples/2 + 1;
    constexpr size_t Ncoeffs_to_multiply = 20;

    types::array_p<Nsamples> input_a;
    types::array_p<Nsamples> input_b;
    for (size_t i = 0; i < input_a.size(); ++i)
    {
        input_a[i] = types::Precision((i*i) % 17);
        input_b[i] = types::Precision((i*i*i) % 23);
    }

    types::array_cp<Ncoeffs> coeffs_a;
    types::array_cp<Ncoeffs> coeffs_b;

    utils::DFT_real_1d<Nsamples> transformer{};
    transformer.reset(-1.0);
    transformer.r2c(input_a, coeffs_a);
    transformer.r2c(input_b, coeffs_b);

    types::array_p<Nsamples> surface;

    auto time_start = time_func();

    for (size_t output_index = 0; output_index < Nsamples; ++output_index)
    {
        types::Precision sum(0.0);
        for (size_t i = 0; i < Ncoeffs_to_multiply; ++i)
        {
            auto a_conj = std::conj(coeffs_a[i]);
            auto b = coeffs_b[i];

            types::Precision freq = output_index * i / types::Precision(domain::WindowSize);

            types::cPrecision term_i = a_conj * b * std::exp(constants::twopij * freq);

            sum += term_i.real();
        }

        surface[output_index] = sum;
    }

    auto time_stop = time_func();
    return time_stop - time_start;
}

// still naive implementation
// compute std::live  (no precompute)
// use half the coefficients by leveraging conjugate symmetry
// use half the outputs since we dont need them
template <typename TimeFuncT>
auto niave_r2c_precompute_exp(const TimeFuncT & time_func)
{
    constexpr size_t Nsamples = domain::WindowSize;
    constexpr size_t Ncoeffs = Nsamples/2 + 1;
    constexpr size_t Ncoeffs_to_multiply = 20;

    types::array_p<Nsamples> input_a;
    types::array_p<Nsamples> input_b;
    for (size_t i = 0; i < input_a.size(); ++i)
    {
        input_a[i] = types::Precision((i*i) % 17);
        input_b[i] = types::Precision((i*i*i) % 23);
    }

    types::array_cp<Ncoeffs> coeffs_a;
    types::array_cp<Ncoeffs> coeffs_b;

    utils::DFT_real_1d<Nsamples> transformer{};
    transformer.reset(-1.0);
    transformer.r2c(input_a, coeffs_a);
    transformer.r2c(input_b, coeffs_b);

    types::array_p<Nsamples> surface;

    std::array<types::array_cp<Ncoeffs>, Nsamples> Wn;
    for (size_t output_index = 0; output_index < Nsamples; ++output_index)
    {
        for (size_t i = 0; i < Ncoeffs; ++i)
        {
            types::Precision freq = (output_index * i) / types::Precision(domain::WindowSize);

            auto a_conj = std::conj(coeffs_a[i]);
            Wn[output_index][i] = a_conj * std::exp(constants::twopij * freq);

        }
    }

    auto time_start = time_func();

    for (size_t output_index = 0; output_index < Nsamples; ++output_index)
    {
        types::Precision sum(0.0);
        for (size_t i = 0; i < Ncoeffs_to_multiply; ++i)
        {
            auto a_conj = std::conj(coeffs_a[i]);
            auto b = coeffs_b[i];

            types::cPrecision term_i =  b * Wn[output_index][i]; //a_conj baked in

            sum += term_i.real();
        }

        surface[output_index] = sum;
    }

    auto time_stop = time_func();
    return time_stop - time_start;
}

// efficient radix2 implementation
template <typename TimeFuncT>
auto fft_r2c_radix2(const TimeFuncT & time_func)
{
    constexpr size_t Nsamples = domain::WindowSize;
    constexpr size_t Ncoeffs = Nsamples/2 + 1;

    types::array_p<Nsamples> input_a;
    types::array_p<Nsamples> input_b;
    for (size_t i = 0; i < input_a.size(); ++i)
    {
        input_a[i] = types::Precision((i*i) % 17);
        input_b[i] = types::Precision((i*i*i) % 23);
    }

    types::array_cp<Ncoeffs> coeffs_a;
    types::array_cp<Ncoeffs> coeffs_b;

    utils::DFT_real_1d<Nsamples> transformer{};
    transformer.reset(-1.0);
    transformer.r2c(input_a, coeffs_a);
    transformer.r2c(input_b, coeffs_b);

    types::array_cp<Ncoeffs> a_conj_b;
    types::array_p<Nsamples> surface;

    auto time_start = time_func();

    for (size_t i = 0; i < Ncoeffs; ++i)
    {
        a_conj_b[i] = std::conj(coeffs_a[i]) * coeffs_b[i];
    }

    transformer.c2r(a_conj_b, surface);

    auto time_stop = time_func();
    return time_stop - time_start;
}