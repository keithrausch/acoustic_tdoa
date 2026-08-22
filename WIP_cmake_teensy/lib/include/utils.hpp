#ifndef DETECTOR_HPP
#define DETECTOR_HPP

// #include <fftw3.h>
// #include "lib/fftw/fftw-3.3.10/api/fftw3.h"
// #include <api_fftw3.h>

#include <algorithm>
#include <limits>
#include <cmath>
// #include <iostream>
#include <iomanip>
// #include <sstream>
#include <type_traits>

#include "constants.hpp"
#include "types.hpp"
#include "fft.hpp"

namespace utils
{
    template <typename T>
    struct is_std_function : std::false_type {};

    template <typename R, typename... Args>
    struct is_std_function<std::function<R(Args...)>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_std_function_v =
        is_std_function<std::remove_cvref_t<T>>::value;

    template <typename T>
    static T sinc(T t)
    {
        return (t != 0) ? std::sin(t) / t : 1.0;
    }

    template <typename T>
    static T sinc2(T t)
    {
        return (t != 0) ? std::sin(t) * std::sin(t) / (t * t) : 1.0;
    }

    struct WaveParams
    {
        types::Precision amplitude{1.0};
        types::Precision center_s{0.0};
        types::Precision freq_hz{1.0 / constants::twopi};
    };

    // create a new set of WaveParams offset in time
    static utils::WaveParams get_offset_params(types::Precision sample_period_s, const utils::WaveParams &chirp_params, types::Precision offset_s)
    {
        types::Precision sinc_insertion_time_s = chirp_params.center_s + offset_s;
        return utils::WaveParams{.amplitude = chirp_params.amplitude, .center_s = sinc_insertion_time_s, .freq_hz = chirp_params.freq_hz};
    }

    template <typename T, typename SoundFunctionT>
    static T generic_sound(T t, const std::vector<WaveParams> &sin_params, const std::vector<std::pair<WaveParams, /*types::*/SoundFunctionT>> &chirp_params)
    {
        T ret = 0.0;
        for (const auto &param : sin_params)
        {
            ret += param.amplitude * std::sin(param.freq_hz * constants::twopi * (t - param.center_s));
        }

        for (const auto &[param, func] : chirp_params)
        {
            if (func)
            {
                ret += param.amplitude * func(param.freq_hz * constants::twopi * (t - param.center_s));
            }
        }

        return ret;
    }

    template <size_t N, typename SoundFunctionT>
    static types::array_p<N> create_template(size_t start_index, types::Precision sample_period_s, const /*types::*/SoundFunctionT &func, const WaveParams &params = WaveParams())
    {
        types::array_p<N> ret;

        if constexpr (is_std_function_v<SoundFunctionT>)
        {
            if (!func)
            {
                return ret;
            }
        }

        for (size_t i = 0; i < N; ++i)
        {
            types::Precision t = (i+start_index) * sample_period_s;
            types::Precision omega = params.freq_hz * constants::twopi;
            ret[i] = params.amplitude * func(omega * (t - params.center_s));
        }

        return ret;
    }

    
    template <typename StreamT, typename ContainerT>
    static void print(const StreamT& ss, const std::string str, const ContainerT &container)
    {
        ss << str + "\n";
        for (size_t i = 0; i < container.size(); ++i)
        {
            ss << i << ") " << container[i] << "\n";
        }
    }

    /// computes 0^0 == 1
    template <size_t p, typename T>
    constexpr auto pow(T val)
    {
        // yes im aware this is bugged for 0^0 but i need that to be 1 anyways
        typedef T U;
        U result{1};
        for (size_t i = 0; i < p; ++i)
        {
            result *= val;
        }
        return result;
    }

    template <size_t Nsamples, size_t max_derivative_order>
    struct CorrelationHelper
    {
        constexpr static size_t NderivativeBuffers = max_derivative_order + 1;
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT; // elements are 2 doubles, so we need half the length
        typedef types::array_p<Nsamples> RealsT;

        types::array_p<Ncoeffs> freqs;
        std::array<types::array_cp<Ncoeffs>, NderivativeBuffers> coeffs_a_for_fft; // shouldnt go past the first derivative
        std::array<types::array_cp<Ncoeffs>, NderivativeBuffers> coeffs_a_for_manual_reconstruction; // shouldnt go past the first derivative

        types::array_cp<Ncoeffs> corr_product;
        std::array<types::array_p<Nsamples>, NderivativeBuffers> correlation_surface; // shouldnt go past the first derivative

        utils::FFT_real_1d<Nsamples> fft;

        constexpr static size_t nyquist_index = Ncoeffs-1; // this is actually N/2 of the full FFT

        // valid for all possible indices [0, Nsamples].
        // output units are integers, not time
        static constexpr int surface_index_to_tau_signed_index(int i)
        {
            int tau = (i >= static_cast<int>(Nsamples/2)) ? (i-static_cast<int>(Nsamples)) : i;
            return tau;
        }

        void setup(const CoeffsT &coeffs_a, types::Precision sample_freq_hz = 1.0)
        {
            static_assert(nyquist_index == Nsamples/2, "the coeffs of an r2c transform put nyquist at the end and are N/2+1 in size. a full c2c transfrom would have N coeffs (with nyquist still at N/2)");
            static_assert(Ncoeffs != Nsamples, "assuming we are using coefficients from an r2c transform");

            fft.reset(+1);


            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                types::Precision freq = i * sample_freq_hz / static_cast<types::Precision>(Nsamples);

                auto a_conj = std::conj(coeffs_a[i]);

                if (i == Ncoeffs-1)
                {
                    freq *= -1;
                }


                coeffs_a_for_fft[0][i] = a_conj;
                coeffs_a_for_manual_reconstruction[0][i] = a_conj;

                bool is_dc_ny = (i == 0) || (i == nyquist_index);
                if (!is_dc_ny)
                {
                    coeffs_a_for_manual_reconstruction[0][i] *= 2.0;
                }

                // coeffs_a_for_fft[0][i] *= 2.0;
                coeffs_a_for_fft[0][i] /= (Nsamples*Nsamples);
                coeffs_a_for_manual_reconstruction[0][i] /= (Nsamples*Nsamples);

                freqs[i] = freq;

                for (size_t o = 1; o < NderivativeBuffers; ++o)
                {
                    coeffs_a_for_fft[o][i] = coeffs_a_for_fft[o-1][i]*freq * constants::twopij;
                    coeffs_a_for_manual_reconstruction[o][i] = coeffs_a_for_manual_reconstruction[o-1][i]*freq * constants::twopi;
                }

                // zero out the Nyquist coefficient for odd ordered derivatives. 
                // we can 0 the dc term too, it was already going to be 0
                //
                // see https://math.mit.edu/~stevenj/fft-deriv.pdf for why we do this. TLDR, the 
                // Nyquist coeff is purely real (for purely real inputs) and, unlike all other 
                // frequencies (except DC), it doesnt have a complex conjugate term to cancel its 
                // imaginary part out. so when we take the derivative for the Nyquist term, 
                // suddenly our derivative gets an imaginary component
                if (is_dc_ny)
                {
                    for (size_t o = 1; o < NderivativeBuffers; o+=2)
                    {
                        coeffs_a_for_fft[o][i] *= 0.0;
                        coeffs_a_for_manual_reconstruction[o][i] *= 0.0;
                    }
                }
            }
        }

        template <size_t derivative_order>
        void set_correlation_surface_via_fft(const CoeffsT &B)
        {
            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                corr_product[i] = coeffs_a_for_fft[derivative_order][i] * B[i];
            }
    

            fft.c2r(corr_product, correlation_surface[derivative_order]);
        }

        template <size_t derivative_order, typename TauT>
        auto correlate_impl(const CoeffsT &B, /*types::Precision duration_s,*/ const TauT tau) const
        {
            types::Precision sum(0.0);
            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                // a_conj baked into coeffs table
                auto b = B[i];

                types::cPrecision term_i = coeffs_a_for_manual_reconstruction[derivative_order][i] * b * std::exp(constants::twopij * freqs[i] * tau);

                // we can get away with pulling only the real/imag parts instead of the total 
                // complex magnitude because these calculations should produce purely real results. 
                // the other component (the one not taken) SHOULD be zero 
                if constexpr(derivative_order % 4 == 0)
                {
                    sum += term_i.real();
                }
                else if constexpr(derivative_order % 4 == 1)
                {
                    sum -= term_i.imag();
                }
                else if constexpr(derivative_order % 4 == 2)
                {
                    sum -= term_i.real();
                }
                else if constexpr(derivative_order % 4 == 3)
                {
                    sum += term_i.imag();
                }
            }

            // sum *= duration_s;
            return sum;
        }

        template <typename IndexT>
        auto correlate_at(const CoeffsT &B, IndexT index_or_tau) const
        {
            return correlate_impl<0>(B, index_or_tau);
        }

        template <typename TauT, size_t... indicesT>
        auto correlate_and_derive_impl(const CoeffsT &B, const TauT tau, std::integer_sequence<size_t, indicesT...>) const
        {
            typedef decltype(correlate_impl<0>(B, tau)) correlation_return_type;
            std::array<correlation_return_type, sizeof...(indicesT)> ret;
            ((ret[indicesT] = correlate_impl<indicesT>(B, tau)), ...);
            return ret;
        }

        template <size_t derivative_order, typename TauT = types::Precision>
        auto correlate_and_derive_v0(const CoeffsT &B, const TauT tau) const
        {
            return correlate_and_derive_impl(B, tau, typename std::make_index_sequence<derivative_order + 1>());
        }

        template <size_t derivative_order, typename TauT = types::Precision>
        auto correlate_and_derive_v1(const CoeffsT &B, const TauT tau) const
        {
            constexpr size_t Norders = derivative_order + 1; // 0th order still does orig function

            types::array_p<Norders> sum;
            sum.fill(0.0);
            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                // a_conj baked into coeffs table
                auto b = B[i];
                auto Wn = std::exp(constants::twopij * freqs[i] * tau);
                auto partial_product = b * Wn;

                for (size_t o = 0; o < Norders; ++o)
                {
                    types::cPrecision term_i = coeffs_a_for_manual_reconstruction[o][i] * partial_product;


                    // we can get away with pulling only the real/imag parts instead of the total 
                    // complex magnitude because these calculations should produce purely real results. 
                    // the other component (the one not taken) SHOULD be zero 
                    if (o % 4 == 0)
                    {
                        sum[o] += term_i.real();
                    }
                    else if (o % 4 == 1)
                    {
                        sum[o] -= term_i.imag();
                    }
                    else if (o % 4 == 2)
                    {
                        sum[o] -= term_i.real();
                    }
                    else /* if (o % 4 == 3) */
                    {
                        sum[o] += term_i.imag();
                    }
                }
            }

            // sum *= duration_s;
            return sum;
        }

        template <size_t derivative_order, typename TauT = types::Precision>
        auto correlate_and_derive_v2(const CoeffsT &B, const TauT tau) const
        {
            constexpr size_t Norders = derivative_order + 1; // 0th order still does orig function

            types::cPrecision Wn(1,0);
            types::cPrecision W1 = std::exp(constants::twopij * freqs[1] * tau);

            types::array_p<Norders> sum;
            sum.fill(0.0);
            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                // a_conj baked into coeffs table
                auto b = B[i];
                auto partial_product = b * Wn;

                for (size_t o = 0; o < Norders; ++o)
                {
                    types::cPrecision term_i = coeffs_a_for_manual_reconstruction[o][i] * partial_product;

                    // we can get away with pulling only the real/imag parts instead of the total 
                    // complex magnitude because these calculations should produce purely real results. 
                    // the other component (the one not taken) SHOULD be zero 
                    if (o % 4 == 0)
                    {
                        sum[o] += term_i.real();
                    }
                    else if (o % 4 == 1)
                    {
                        sum[o] -= term_i.imag();
                    }
                    else if (o % 4 == 2)
                    {
                        sum[o] -= term_i.real();
                    }
                    else /* if (o % 4 == 3) */
                    {
                        sum[o] += term_i.imag();
                    }
                }

                Wn *= W1;
            }

            // sum *= duration_s;
            return sum;
        }
    
        
        template <size_t derivative_order, typename TauT = types::Precision>
        auto correlate_and_derive(const CoeffsT &B, const TauT tau) const
        {
            return correlate_and_derive_v2<derivative_order>(B, tau);
        }
    };

    template <size_t Nsamples>
    struct FFTHelper
    {
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT; // elements are 2 doubles, so we need half the length
        typedef types::array_p<Nsamples> RealsT;

        FFT_real_1d<Nsamples> fft{};
        RealsT input{};
        CoeffsT coeffs{};

        FFTHelper() = default;
        FFTHelper(FFTHelper &&rhs) = default;

        void reset()
        {
            input.fill(0.0);
            coeffs.fill(types::cPrecision(0.0, 0.0));

            fft.reset();
        }

        void transform()
        {

            fft.r2c(input, coeffs);
        }

        // result is unscaled
        // can accept fractional index
        // inefficient if computing multiple times, just use c2r for that
        template <typename T>
        types::Precision manually_reconstruct_at_index(T k)
        {
            types::cPrecision Wn(1,0);
            types::Precision freq_1 = static_cast<types::Precision>(1) / Nsamples;
            types::cPrecision W1 = std::exp(constants::twopij * (freq_1 * k));

            types::Precision sum{};
            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                types::cPrecision term_i = coeffs[i] * Wn;
                if ((i > 0) && (i < Ncoeffs-1)) // can pull this check out front if -O3 isnt doing that already
                {
                    term_i *= 2;
                }
                sum += term_i.real();
                Wn *= W1;
            }

            // sum *= duration_s;
            return sum;
        }

        template <typename SoundFunctionT>
        static FFTHelper construct_simple(types::Precision sample_period_s, const /*types::*/SoundFunctionT &chirp_func, const WaveParams &chirp_params)
        {
            utils::FFTHelper<Nsamples> chirp;
            chirp.reset();
            chirp.input = utils::create_template<Nsamples>(0, sample_period_s, chirp_func, chirp_params);
            chirp.transform();

            return chirp;
        }
    };


    template <typename CallableT>
    std::pair<types::Precision, types::Precision> newton(const CallableT &fd0_fd1_fd2, types::Precision guess_tau_s, size_t n_steps = 3)
    {
        types::Precision x_n = guess_tau_s;
        for (size_t i = 0; i < n_steps; ++i)
        {
            auto [f_d0, f_d1, f_d2] = fd0_fd1_fd2(x_n);
            auto update = f_d1 / f_d2;
            // std::cout << "update: " << update << "\n";
            // double stopping_thresh = std::numeric_limits<double>::epsilon() * f_d1.real();
            // if (std::abs(update) < stopping_thresh)
            // {
            //     break;
            // }
            x_n = x_n - update;
            // eval_and_print(x_n);
        }

        auto [f_d0, f_d1, f_d2] = fd0_fd1_fd2(x_n);
        return std::make_pair(x_n, f_d0);
    };

    template <typename IndexT=uint32_t, typename ValueT=float>
    class ExtremmaFinder
    {
        public:

        struct CandidateExtremma
        {
            IndexT time_idx{};
            float time_idx_fraction{}; // hopefully bounded by [-1,+1] but not guaranteed
            ValueT value{0.0};

            void reset()
            {
                *this = CandidateExtremma();
            }

            // using sample_freq_hz instead of sample_period_s for precision at the cost of speed
            types::Precision to_time_s(types::Precision sample_freq_hz) const
            {
                return static_cast<types::Precision>(time_idx)/sample_freq_hz + static_cast<types::Precision>(time_idx_fraction)/sample_freq_hz;
            }
        };

        typedef std::vector<CandidateExtremma> vector_extremma;

        const vector_extremma & extremma()
        {
            return extremma_;
        }

        void resize(size_t n_extremma_in)
        {
            n_extremma = n_extremma_in;
            extremma_.resize(n_extremma);
        }

        void reset(size_t n_extremma_in)
        {
            resize(n_extremma_in);

            // reset results container. each element is default constructed

            last_f_d1_sign = 2; // impossible value, will always record first sample as a peak
        }

        void prune_before(IndexT prune_time_idx)
        {
            for (auto & ex : extremma_)
            {
                if (ex.time_idx < prune_time_idx)
                {
                    ex.reset();
                }
            }

            move_invalids_to_back();
        }

        // biased so the weaker peaks get cleared and leave the stronger ones
        void filter_redundant(IndexT tolerance_idx)
        {
            for (int i = extremma_.size()-1; i > 0; --i)
            {
                for (int j = i-1; j >= 0; --j)
                {
                    auto a = extremma_[i].time_idx;
                    auto b = extremma_[j].time_idx;
                    bool is_within_tolerance = ((a > b) ? (a-b) : (b-a)) <= tolerance_idx;
                    if (is_within_tolerance)
                    {
                        extremma_[i].reset();
                        break;
                    }
                }
            }

            move_invalids_to_back();
        }

        template <size_t Nsamples, size_t max_derivative_order, size_t Ncoeffs>
        size_t find_extremma(const CorrelationHelper<Nsamples, max_derivative_order>& correlation_helper, const types::array_cp<Ncoeffs> &B, IndexT tau_to_time_idx)
        {
            // define tau search bounds
            constexpr size_t idx_fh_start = Nsamples/2 + Nsamples/4; // most negative tau
            constexpr size_t idx_fh_stop = Nsamples; // least negative tau
            constexpr size_t idx_sh_start = 0; // least positive tau (and 0)
            constexpr size_t idx_sh_stop = Nsamples/4; // most positive tau

            // this is an over-fancy way of saying N/4
            constexpr int index_offset_signed = CorrelationHelper<Nsamples, max_derivative_order>::surface_index_to_tau_signed_index(idx_fh_start);
            constexpr size_t index_offset_abs = std::abs(index_offset_signed);
            static_assert(index_offset_signed == -static_cast<int>(Nsamples)/4);
            static_assert(index_offset_abs == Nsamples/4);

            // equivelant to CorrelationHelper::index_to_tau(), but offset by Nsamples/4 so the return 
            // value cant be negative. we only search a half a window in size, but its centered at the endpoints so we only need to add N/4
            // auto index_to_tau_offset = [](size_t i) -> size_t
            // {
            //     size_t tau = (i >= Nsamples/2) ? (i-Nsamples + index_offset_abs) : (i + index_offset_abs); 
            //     return tau;
            // }


            // we want tau_index to always be positive so we can use uint64_t for time_index and 
            // tau_index. so we can force tau_index to always be positive by adding N/2 to it and 
            // subtracting N/2 here to makeup for it
            if (tau_to_time_idx < static_cast<IndexT>(index_offset_abs)) // TODO handle this better. somehow enforce this can never happen
            {
                return 0;
            }

            IndexT shifted_tau_to_time_idx = tau_to_time_idx - static_cast<IndexT>(index_offset_abs); // this offset added back in later with index_to_tau_offset_for_unsigned()

            auto sign = [](types::Precision v)
            {
                // old, we can make it faster...
                // return static_cast<SignT>((v > 0.0) - (v < 0.0));

                // NOTE this is will return only 1 or 0, (v < 0). not -1,0,+1
                return static_cast<SignT>(std::signbit(v)); 
            };

            auto fd0_fd1_fd2 = [&correlation_helper, &B](auto tau)
            { return correlation_helper.template correlate_and_derive<2>(B, tau); };

            auto check_index = [&](size_t i, size_t tau_idx_shifted)
            {
                auto f_d1 = correlation_helper.correlation_surface[1][i];
                auto f_d1_sign = sign(f_d1);

                if (f_d1_sign != last_f_d1_sign)
                {
                    // auto tau_idx_shifted_positive = static_cast<IndexT>(correlation_helper.index_to_tau_offset(i)); // offset 0 to N/2
                    auto f_d0 = correlation_helper.correlation_surface[0][i];
                    auto f_d0_abs = std::abs(static_cast<ValueT>(f_d0));
                    record_extremma_in_min_heap(CandidateExtremma{.time_idx = static_cast<IndexT>(tau_idx_shifted) + shifted_tau_to_time_idx, .value = f_d0_abs});
                    last_f_d1_sign = f_d1_sign;
                }
            };

            // actually perform the search. 
            // first over negative tau (most negative to least negative)
            for (size_t i = idx_fh_start; i < idx_fh_stop; ++i)
            {
                // size_t tau_idx = i - Nsamples; // tau from -N/4 to -1 (almost 0)
                size_t tau_idx_shifted = i - Nsamples + index_offset_abs; // tau from 0 to N/4-1 (almost N/4)
                check_index(i, tau_idx_shifted);
            }
        
            // now from 0 to most positive
            for (size_t i = idx_sh_start; i < idx_sh_stop; ++i)
            {
                // size_ tau_idx = i; // tau from 0 to N/4
                size_t tau_idx_shifted = i + index_offset_abs; // tau from N/4 to N/2
                check_index(i, tau_idx_shifted);
            }
            

            // refine the peaks that were found
            size_t n_extremma_added = 0;
            auto time_idx_lower_bound = shifted_tau_to_time_idx;
            for (auto &[time_idx, time_idx_fraction, value] : extremma_)
            {
                if (time_idx == 0)
                {
                    continue;
                }

                if (time_idx < time_idx_lower_bound)
                {
                    continue;
                }

                auto tau_idx_shifted = static_cast<size_t>(time_idx - shifted_tau_to_time_idx); // 0 to N/2
                int tau_idx_signed = tau_idx_shifted - index_offset_abs; // tau from -N/4 to +N/4

                types::Precision tau_index_signed_float = tau_idx_signed;
                auto [optimal_tau_idx, optimal_f_d0] = utils::newton(fd0_fd1_fd2, tau_index_signed_float);
                time_idx_fraction = optimal_tau_idx - tau_index_signed_float; // overwrite
                value = std::abs(optimal_f_d0);  // overwrite
                ++n_extremma_added;
            }

            return n_extremma_added;
        }

        template <typename StreamT>
        void print(StreamT & out, types::Precision sample_period_s, IndexT subtract_this=0.0)
        {
            // std::stringstream out;
            for (size_t i = 0; i < extremma_.size(); ++i)
            {
                auto &[time_idx, time_idx_fractional, value] = extremma_[i];
                out << "peak " << i << ") ";

                {
                    std::stringstream ss;
                    ss << "tau_index: ";
                    ss << std::fixed << std::showpoint << std::showpos;
                    ss << std::setprecision(6);
                    // ss << std::setw(10) << std::setfill('0');
                    if (time_idx > 0)
                    {
                        ss << static_cast<int64_t>(time_idx)-subtract_this << "[] ";
                        ss << std::setprecision(8);
                        ss << "fractional_index: " << time_idx_fractional << " ";
                        ss << ", time_s: " << (static_cast<int64_t>(time_idx)-subtract_this+time_idx_fractional)*sample_period_s<< "s";
                    }
                    else
                    {
                        ss << "***[] fractional_index: *********** time_s: ***********s";
                    }
                    // out << ss.str();
                }

                {
                    std::stringstream ss;
                    ss << std::scientific << std::showpos;
                    ss << std::setprecision(8);
                    if (time_idx > 0)
                    {
                        ss << ", value: " << value;
                    }
                    else
                    {
                        ss << ", value: **************s";
                    }
                    // out << ss.str();
                }

                // {
                //     std::stringstream ss;
                //     // ss << std::scientific << std::showpos;
                //     // ss << std::setprecision(8);
                //     ss << " id:" << id << "\n";
                //     std::cout << ss.str();
                // }
                out << "\n";
            }

            // auto str = out.str();
            // std::cout << str;
            // return str;
        }
        
    
    private:
        size_t n_extremma = 2 * 5 + 1; // because likely symmetry

        typedef bool SignT;
        SignT last_f_d1_sign{};
        types::Precision last_tau_s{};

        vector_extremma extremma_;
        size_t count{0};

        void record_extremma_in_min_heap(const CandidateExtremma & candidate)
        {
            auto new_abs_value = /*std::abs*/(candidate.value);
            if (new_abs_value < /*std::abs*/(extremma_.back().value))
            {
                return;
            }

            int i = n_extremma - 1; // need the sign

            while ((i > 0) && (new_abs_value > /*std::abs*/(extremma_[i-1].value)))
            {
                extremma_[i] = extremma_[i-1];
                --i;
            }

            extremma_[i] = candidate;
        }

        void move_invalids_to_back()
        {

            auto it = 
            std::remove_if(extremma_.begin(), 
                              extremma_.end(),
                              [](const CandidateExtremma& x) { return x.time_idx == 0; });
            
            while (it != extremma_.end())
            {
                it->reset();
                ++it;
            }

        }
    };

    template <size_t WindowSize, size_t Nchannels=1, typename IndexT=uint32_t, typename PeakT=float>
    class Ingestor
    {
        public:

        typedef utils::ExtremmaFinder<IndexT, PeakT> ExtremmaFinderT;
        typedef typename ExtremmaFinderT::vector_extremma vector_extremma;
        typedef std::pair<IndexT, IndexT> time_bounds;

        struct types {
            using index_type = IndexT;
            using peak_type  = PeakT;
        };

        protected:
        static constexpr size_t BlockSize = WindowSize / 2;

        // types::Precision cd_freq_hz{};      // = 44100.0;
        // types::Precision sample_period_s{}; // = 1.0 / cd_freq_hz;
        utils::FFTHelper<WindowSize> chirp{};
        std::array<utils::FFTHelper<WindowSize>, Nchannels> signals{};
        utils::CorrelationHelper<WindowSize, 2> correlation_helper{};
        std::array<ExtremmaFinderT, Nchannels> extremma_helpers_{};

        public:

        template <size_t channel_index=0>
        ExtremmaFinderT & extremma_helper()
        {
            return extremma_helpers_[channel_index];
        }

        // time_bounds tau_bounds_idx()
        // {
        //     return time_bounds_idx(0);
        // }

        time_bounds time_bounds_idx(IndexT tau_to_time_offset_idx)
        {
            return std::make_pair(tau_to_time_offset_idx  - BlockSize/2, 
                                  tau_to_time_offset_idx  + BlockSize/2);
        }

        void reset(const typename utils::FFTHelper<WindowSize>::RealsT &chirp_input)
        {
            // cd_freq_hz = cd_freq_hz_in;
            // sample_period_s = 1.0 / cd_freq_hz;

            chirp.reset();
            chirp.input = chirp_input;
            chirp.transform();

            for (size_t channel_index = 0; channel_index < Nchannels; ++channel_index)
            {
                signals[channel_index].reset();
            }

            correlation_helper.setup(chirp.coeffs);
        }

        // template <size_t derivative_order, typename TauT>
        // auto correlate_and_derive(TauT tau)
        // {
        //     // auto window_period_s = WindowSize * sample_period_s;
        //     return correlation_helper.template correlate_and_derive<derivative_order>(signal.coeffs, tau);
        // }

        template <typename dataInT>
        size_t run(size_t channel_index, dataInT *src, IndexT tau_to_time_offset_idx, size_t n_extremma, bool reset_heap = true)
        {
            if (channel_index >= Nchannels)
            {
                return 0;
            }

            if (!src)
            {
                return 0;
            }

            auto & signal = signals[channel_index];
            auto & extremma_helper = extremma_helpers_[channel_index];

            // roll data
            for (size_t i = 0; i < BlockSize; ++i)
            {
                signal.input[i] = signal.input[i + BlockSize];
                signal.input[i + BlockSize] = src[i];
            }

            signal.transform();

            correlation_helper.template set_correlation_surface_via_fft<0>(signal.coeffs);
            correlation_helper.template set_correlation_surface_via_fft<1>(signal.coeffs);

            // implement a search
            // auto tau_s_step = sample_period_s;
            extremma_helper.resize(n_extremma);
            if (reset_heap)
            {
                extremma_helper.reset(n_extremma);
            }
            return extremma_helper.find_extremma(correlation_helper, signal.coeffs, tau_to_time_offset_idx);
        }
    };

    template <size_t WindowSize, size_t Nchannels=1, typename IndexT=uint32_t, typename PeakT=float>
    class SignalAcquirer : public Ingestor<WindowSize, Nchannels, IndexT, PeakT>
    {
        public:
        struct types {
            using index_type = IndexT;
            using peak_type  = PeakT;
        };

        static constexpr size_t nDetsForHypothesis = 3;

        template <typename dataInT, typename CallbackT>
        size_t run(size_t channel_index, dataInT *src, const CallbackT & callback, IndexT tau_to_time_offset_idx, size_t n_extremma, size_t sync_period_idx, size_t sync_half_gate_idx, size_t nearby_peak_tolerance_idx)
        {
            if (channel_index >= Nchannels)
            {
                return 0;
            }

            auto & extremma_helper = this->extremma_helpers_[channel_index];

            [[maybe_unused]]
            auto n_extremma_added = Ingestor<WindowSize, Nchannels>::run(channel_index, src, tau_to_time_offset_idx, n_extremma, false);

            const auto & extremma = extremma_helper.extremma();

            // prune dets that are way too old
            IndexT window_size_idx = nDetsForHypothesis * (sync_period_idx + sync_half_gate_idx);
            if (window_size_idx > 0)
            {
                auto prune_time_idx = (tau_to_time_offset_idx > window_size_idx) ? (tau_to_time_offset_idx - window_size_idx) : 0;
                extremma_helper.prune_before(prune_time_idx);
            }
            if (nearby_peak_tolerance_idx > 0)
            {
                extremma_helper.filter_redundant(nearby_peak_tolerance_idx);
            }

            if (0 == n_extremma_added)
            {
                return n_extremma_added;
            }
            
            fire_on_new_peak(channel_index, extremma, callback, tau_to_time_offset_idx, sync_period_idx, sync_half_gate_idx);

            return n_extremma_added;
        }

        template <typename CallbackT>
        void fire_on_new_peak(size_t channel_index, const typename Ingestor<WindowSize, Nchannels>::vector_extremma & extremma, const CallbackT & callback, IndexT tau_to_time_offset_idx, size_t sync_period_idx, size_t sync_half_gate_idx)
        {
            // get the detection that was just added
            // only need to search the top N, even if the points just added arent in the top N
            hypothesis.fill(-1);
            extremma_mask.resize(extremma.size());
            for (size_t i = 0; i < extremma_mask.size(); ++i)
            {
                extremma_mask[i] = false;
            }
            static_assert(nDetsForHypothesis > 0, "array size too small");
            auto [next_youngest_time_idx_lower_bound, next_youngest_time_idx_upper_bound] = this->time_bounds_idx(tau_to_time_offset_idx);
            IndexT youngest_det_time_idx = 0;
            size_t hypothesis_size = 0;

            auto update_time_and_gate = [&](size_t i)
            {
                hypothesis[hypothesis_size++] = i;
                extremma_mask[i] = true;

                youngest_det_time_idx = extremma[i].time_idx;
                next_youngest_time_idx_lower_bound = youngest_det_time_idx - sync_period_idx - sync_half_gate_idx;
                next_youngest_time_idx_upper_bound = youngest_det_time_idx - sync_period_idx + sync_half_gate_idx;
            };

            // auto tau_s_step = this->sample_period_s;
            for (size_t i = 0; i < extremma.size(); ++i)
            {
                auto det_time_idx = extremma[i].time_idx;
                if (det_time_idx >= next_youngest_time_idx_lower_bound - /*tau_s_step*/1) // detection peak could be between blocks
                {
                    update_time_and_gate(i);
                    break;
                }
            }

            if (0 == hypothesis_size)
            {
                return;
            }

            for (int i = 0; i < static_cast<int>(extremma.size()); ++i)
            {
                if (extremma_mask[i])
                {
                    continue;
                }

                const auto & det = extremma[i];
                auto det_time_idx = det.time_idx;
                if (det_time_idx >= next_youngest_time_idx_lower_bound && det_time_idx <= next_youngest_time_idx_upper_bound)
                {
                    update_time_and_gate(i);
                    i = -1; // will be incremented back to 0 when the loop iterates
                }

                if (hypothesis.size() == hypothesis_size)
                {
                    break;
                }
            }

            if (nDetsForHypothesis == hypothesis_size)
            {
                callback(channel_index, extremma[hypothesis[0]]);
                // std::cout << "fire callback!\n";
                // std::stringstream ss;
                // for (size_t i = 0; i < hypothesis_size; ++i)
                // {
                //     const auto & det = extremma[hypothesis[i]];
                //     ss <<  i << ")" << " time_s: " << det.time_s << " value: " << det.value<< " id: " << det.id << "\n";
                // }
                // std::cout << ss.str();
            }
        }

        private:
        std::array<int, nDetsForHypothesis> hypothesis;
        std::vector<bool> extremma_mask;
    };

    constexpr uint64_t nCr(size_t n, size_t r = 2) 
    {
        if (r+r > n)
        {
            r = n - r; // because C(n, r) == C(n, n - r)
        } 
        
        uint64_t ans = 1;
    
    
        for (size_t i = 1; i <= r; ++i)
        {
            ans *= (n+i) - r;
            ans /= i;
        }
    
        return ans;
    }
}



#endif
