#ifndef DETECTOR_HPP
#define DETECTOR_HPP

// #include <fftw3.h>
// #include "lib/fftw/fftw-3.3.10/api/fftw3.h"
// #include <api_fftw3.h>

#include <algorithm>
#include <limits>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "constants.hpp"
#include "types.hpp"
#include "fft.hpp"

namespace utils
{

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

    template <typename T>
    static T generic_sound(T t, const std::vector<WaveParams> &sin_params, const std::vector<std::pair<WaveParams, types::SoundFunctionT>> &chirp_params)
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

    template <size_t N>
    static types::array_p<N> create_template(size_t start_index, types::Precision sample_period_s, const types::SoundFunctionT &func, const WaveParams &params = WaveParams())
    {
        types::array_p<N> ret;

        if (!func)
        {
            return ret;
        }

        for (size_t i = 0; i < N; ++i)
        {
            types::Precision t = (i+start_index) * sample_period_s;
            types::Precision omega = params.freq_hz * constants::twopi;
            ret[i] = params.amplitude * func(omega * (t - params.center_s));
        }

        return ret;
    }


    template <typename ContainerT>
    static void print(const std::string str, const ContainerT &container)
    {
        std::cout << str + "\n";
        for (size_t i = 0; i < container.size(); ++i)
        {
            std::cout << i << ") " << container[i] << "\n";
        }
    }

    template <size_t p, typename T>
    // concept so p is >= 0
    constexpr auto pow(T val)
    {
        // yes im aware this is bugged for 0^0 but i need that to be 1 anyways
        // typedef std::remove_cvref<T>::type U;
        typedef T U;
        U result{1};
        for (size_t i = 0; i < p; ++i)
        {
            result *= val;
        }
        return result;
    }

    template <size_t Ncoeffs>
    static types::Precision reconstruct_at_index(const types::array_cp<Ncoeffs> &coeffs, size_t k)
    {
        constexpr types::Precision Nsamples = Ncoeffs_to_Nsamples(Ncoeffs);

        types::Precision sum(0.0);
        for (size_t m = 0; m < coeffs.size()-1; ++m)
        {
            sum += (coeffs[m] * std::exp(constants::twopij * static_cast<types::Precision>(k * m) / Nsamples)).real();
        }
        return sum;
    }

    template <size_t Nsamples, size_t max_derivative_order>
    struct DerivativeHelper
    {
        constexpr static size_t NderivativeBuffers = max_derivative_order + 1;
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT; // elements are 2 doubles, so we need half the length
        typedef types::array_p<Nsamples> RealsT;

        types::array_p<Ncoeffs> freqs;
        std::array<types::array_cp<Ncoeffs>, NderivativeBuffers> coeffs_a_for_dft; // shouldnt go past the first derivative
        std::array<types::array_cp<Ncoeffs>, NderivativeBuffers> coeffs_a_for_manual_reconstruction; // shouldnt go past the first derivative
        
        // std::array<types::array_cp<Ncoeffs>, Nsamples> precomputed_for_index;

        types::array_cp<Ncoeffs> corr_product;
        std::array<types::array_p<Nsamples>, NderivativeBuffers> correlation_surface; // shouldnt go past the first derivative

        utils::DFT_real_1d<Nsamples> dft;


        constexpr static size_t start_fh = Nsamples/2 + Nsamples/4;
        constexpr static size_t stop_fh = Nsamples;
        constexpr static size_t start_sh = 0;
        constexpr static size_t stop_sh = Nsamples/4;

        constexpr static size_t nyquist_index = Ncoeffs-1; // this is actually N/2 of the full DFT

        types::Precision index_to_tau(int i, types::Precision sample_period) const
        {
            auto tau_s_step = sample_period;
            auto tau_s = (i >= static_cast<int>(Nsamples/2)) ? tau_s_step*(i-static_cast<int>(Nsamples)) : i*tau_s_step;
            return tau_s;

        }

        void setup(const CoeffsT &coeffs_a, types::Precision sample_period)
        {
            dft.reset(+1);


            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                types::Precision freq = i * sample_period / static_cast<types::Precision>(Nsamples);

                auto a_conj = std::conj(coeffs_a[i]);

                if (i == Ncoeffs-1)
                {
                    freq *= -1;
                }


                coeffs_a_for_dft[0][i] = a_conj;
                coeffs_a_for_manual_reconstruction[0][i] = a_conj;

                bool is_dc_ny = (i == 0) || (i == nyquist_index);
                if (!is_dc_ny)
                {
                    coeffs_a_for_manual_reconstruction[0][i] *= 2.0;
                }

                // coeffs_a_for_dft[0][i] *= 2.0;
                coeffs_a_for_dft[0][i] /= (Nsamples*Nsamples);
                coeffs_a_for_manual_reconstruction[0][i] /= (Nsamples*Nsamples);

                freqs[i] = freq;

                for (size_t o = 1; o < NderivativeBuffers; ++o)
                {
                    coeffs_a_for_dft[o][i] = coeffs_a_for_dft[o-1][i]*freq * constants::twopij;

                    if (is_dc_ny && (o % 2 == 1)) // only apply to odd ordered derivatives. DC is fine too.
                    {
                        // just throw the Nyquist term away. its cool. it keeps things real ;)
                        // https://math.mit.edu/~stevenj/fft-deriv.pdf
                        coeffs_a_for_dft[o][i] *= 0.0;
                    }

                    coeffs_a_for_manual_reconstruction[o][i] = coeffs_a_for_manual_reconstruction[o-1][i]*freq * constants::twopi;
                }
            }

            // for (size_t i = 0; i < Nsamples; ++i)
            // {
            //     auto tau = index_to_tau(i, 1.0/sample_period);
            //     for (size_t k = 0; k < Ncoeffs; ++k)
            //     {
            //         precomputed_for_index[i][k] = coeffs_a_for_manual_reconstruction[0][k] * std::exp(constants::twopij * freqs[k] * tau);
            //     }
            // }
        }

        template <size_t derivative_order>
        void correlate_via_dft(const CoeffsT &B)
        {
            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                corr_product[i] = coeffs_a_for_dft[derivative_order][i] * B[i];
            }
    

            dft.c2r(corr_product, correlation_surface[derivative_order]);
        }

        template <size_t derivative_order, typename TauT>
        auto correlate_impl(const CoeffsT &B, /*types::Precision duration_s,*/ const TauT tau) const
        {
            // normally we would sum all the terms, but the Nyquist coefficient for odd derivatives actually needs to be 0
            constexpr size_t Ncoeffs_to_sum = Ncoeffs - (derivative_order % 2 == 1);

            types::Precision sum(0.0);
            for (size_t i = 0; i < Ncoeffs_to_sum; ++i)
            {
                // auto a_conj = coeffs_a_for_manual_reconstruction[i];
                auto b = B[i];

                types::cPrecision term_i = /*a_conj * */ coeffs_a_for_manual_reconstruction[derivative_order][i] * b * std::exp(constants::twopij * freqs[i] * tau);

                if constexpr(0 == derivative_order)
                {
                    sum += term_i.real();// * freqs_to_power[derivative_order][i];
                }
                else if constexpr(1 == derivative_order)
                {
                    sum -= term_i.imag();// * freqs_to_power[derivative_order][i];
                }
                else if constexpr(2 == derivative_order)
                {
                    sum -= term_i.real();// * freqs_to_power[derivative_order][i];
                }
            }

            // sum *= duration_s;
            return sum;
        }

        template <typename IndexT>
        auto correlate_at(const CoeffsT &B, IndexT index_or_tau) const
        {
            // types::Precision sum(0.0);
            // for (size_t i = 0; i < Ncoeffs; ++i)
            // {
            //     auto b = B[i];

            //     types::cPrecision term_i = b * precomputed_for_index[index_or_tau][i];

            //     sum += term_i.real();
            // }

            // // sum *= duration_s;
            // return sum;

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
        auto correlate_and_derive(const CoeffsT &B, const TauT tau) const
        {
            return correlate_and_derive_impl(B, tau, typename std::make_index_sequence<derivative_order + 1>());
        }

        // template <typename TauT>
        // auto eval_and_print(const CoeffsT &B, types::Precision duration_s, const TauT tau)
        // {
        //     auto [f, f_d1, f_d2] = correlate_and_derive<2, true>(B, duration_s, tau);

        //     auto sample_period_s = duration_s / Nsamples;

        //     {
        //         std::stringstream ss;
        //         ss << std::fixed << std::showpoint << std::showpos;
        //         ss << std::setprecision(6);
        //         ss << "tau_index:" << tau / sample_period_s;
        //         ss << std::setprecision(8);
        //         ss << " (" << tau << "s)";
        //         std::cout << ss.str();
        //     }

        //     {
        //         std::stringstream ss;
        //         ss << std::scientific << std::showpos;
        //         ss << std::setprecision(8);
        //         ss << ". f: " << f << ", f_d1: " << f_d1 << ", f_d2:" << f_d2 << "\n";
        //         std::cout << ss.str();
        //     }
        // }
    };

    template <size_t Nsamples>
    struct FFTHelper
    {
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT; // elements are 2 doubles, so we need half the length
        typedef types::array_p<Nsamples> RealsT;

        DFT_real_1d<Nsamples> dft;
        RealsT input;
        CoeffsT coeffs;

        FFTHelper() = default;
        FFTHelper(FFTHelper &&rhs) = default;

        void reset()
        {
            input.fill(0.0);
            coeffs.fill(types::cPrecision(0.0, 0.0));

            dft.reset();
        }

        void transform()
        {

            dft.r2c(input, coeffs);
        }

        static FFTHelper construct_simple(types::Precision sample_period_s, const types::SoundFunctionT &chirp_func, const WaveParams &chirp_params)
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
            // if (std::fabs(update) < stopping_thresh)
            // {
            //     break;
            // }
            x_n = x_n - update;
            // eval_and_print(x_n);
        }

        auto [f_d0, f_d1, f_d2] = fd0_fd1_fd2(x_n);
        return std::make_pair(x_n, f_d0);
    };

    class ExtremmaFinder
    {
        public:

        struct CandidateExtremma
        {
            types::Precision time_s{std::numeric_limits<types::Precision>::quiet_NaN()};
            types::Precision value{0.0};
            // size_t id{};

            void reset()
            {
                *this = CandidateExtremma();
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

            last_f_d0_abs = types::Precision{-1};
            last_f_d1_sign = 0;
            last_tau_s = 0;
        }

        void prune_before(types::Precision prune_time_s)
        {
            for (auto & ex : extremma_)
            {
                if (ex.time_s < prune_time_s)
                {
                    ex.reset();
                }
            }

            move_nans_to_back();
        }

        void filter_redundant(types::Precision tolerance_s)
        {
            for (int i = extremma_.size()-1; i > 0; --i)
            {
                for (int j = i-1; j >= 0; --j)
                {
                    bool is_within_tolerance = std::fabs(extremma_[i].time_s - extremma_[j].time_s) <= tolerance_s;
                    if (is_within_tolerance)
                    {
                        extremma_[i].reset();
                    }
                }
            }

            move_nans_to_back();
        }

        template <size_t Nsamples, size_t max_derivative_order, size_t Ncoeffs>
        size_t find_extremma(const DerivativeHelper<Nsamples, max_derivative_order>& derivative_helper, const types::array_cp<Ncoeffs> &B, types::Precision tau_s_lower_bound, types::Precision tau_s_upper_bound, types::Precision tau_s_step, types::Precision tau_to_time_offset_s=std::numeric_limits<types::Precision>::quiet_NaN())
        {
            bool offset_provided = !std::isnan(tau_to_time_offset_s);
            if (!offset_provided)
            {
                tau_to_time_offset_s = 0.0;
            }

            auto sign = [](types::Precision v)
            {
                return (v > 0.0) - (v < 0.0);
            };

            auto fd0_fd1_fd2 = [&](auto tau)
            { return derivative_helper.template correlate_and_derive<2>(B, tau); };

            auto check_index = [&](size_t i)
            {
                auto f_d1 = derivative_helper.correlation_surface[1][i];
                auto f_d1_sign = sign(f_d1);

                if (f_d1_sign != last_f_d1_sign)
                {
                    auto tau_s = derivative_helper.index_to_tau(i, tau_s_step);
                    auto f_d0 = derivative_helper.correlation_surface[0][i];
                    // auto f_d0 = derivative_helper.correlate_at(B, tau_s);
                    // auto f_d0 = derivative_helper.correlate_at(B, i);
                    auto f_d0_abs = std::fabs(f_d0);
                    // auto [fd0_, fd1_, fd2_] = fd0_fd1_fd2(tau_s);
                    record_extremma(CandidateExtremma{.time_s = tau_s+tau_to_time_offset_s, .value = f_d0_abs});
                    last_f_d1_sign = f_d1_sign;
                }
            };

            {
                for (size_t i = derivative_helper.start_fh; i < derivative_helper.stop_fh; ++i)
                {
                    check_index(i);
                }
            }
            {
                for (size_t i = derivative_helper.start_sh; i < derivative_helper.stop_sh; ++i)
                {
                    check_index(i);
                }
            }

            // refine the peaks that were found
            size_t n_extremma_added = 0;
            auto time_s_lower_bound = tau_s_lower_bound + tau_to_time_offset_s;
            for (auto &[time_s, value/*, id*/] : extremma_)
            {
                if (std::isnan(time_s))
                {
                    continue;
                }

                if (offset_provided && time_s < time_s_lower_bound) // funny story, doing this comparrison with tau instead of time runs into machine precision issues when close to the lower bound
                {
                    continue;
                }
                
                auto tau_s = time_s - tau_to_time_offset_s;
                auto [optimal_tau_s, optimal_f_d0] = utils::newton(fd0_fd1_fd2, tau_s);
                time_s = optimal_tau_s + tau_to_time_offset_s; // overwrite
                value = optimal_f_d0;  // overwrite
                ++n_extremma_added;
            }

            return n_extremma_added;
        }

        std::string print(types::Precision sample_period_s, types::Precision subtract_this=0.0)
        {
            std::stringstream out;
            for (size_t i = 0; i < extremma_.size(); ++i)
            {
                auto &[time_s, value/*, id*/] = extremma_[i];
                out << "peak " << i << ") ";

                {
                    std::stringstream ss;
                    ss << "tau_index: ";
                    ss << std::fixed << std::showpoint << std::showpos;
                    ss << std::setprecision(6);
                    // ss << std::setw(10) << std::setfill('0');
                    ss << (time_s-subtract_this)/sample_period_s << "[] ";
                    ss << std::setprecision(8);
                    ss << "time_s: " << time_s << "s";
                    out << ss.str();
                }

                {
                    std::stringstream ss;
                    ss << std::scientific << std::showpos;
                    ss << std::setprecision(8);
                    ss << " value:" << value;
                    out << ss.str();
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

            auto str = out.str();
            std::cout << str;
            return str;
        }
    
    private:
        size_t n_extremma = 2 * 5 + 1; // because likely symmetry

        types::Precision last_f_d0_abs{};
        int last_f_d1_sign{};
        types::Precision last_tau_s{};

        vector_extremma extremma_;
        size_t count{0};

        void record_extremma(/* copy*/ CandidateExtremma this_pair)
        {
            if (std::fabs(this_pair.value) >= std::fabs(extremma_.back().value))
            {
                for (size_t i = 0; i < n_extremma; ++i)
                {
                    if (std::fabs(this_pair.value) >= std::fabs(extremma_[i].value))
                    {
                        auto temp_pair = extremma_[i];
                        extremma_[i] = this_pair;
                        this_pair = temp_pair;
                    }
                }
            }
        }

        void move_nans_to_back()
        {

            auto it = 
            std::remove_if(extremma_.begin(), 
                              extremma_.end(),
                              [](const CandidateExtremma& x) { return std::isnan(x.time_s); });
            
            while (it != extremma_.end())
            {
                it->reset();
                ++it;
            }

        }
    };

    template <size_t WindowSize, size_t Nchannels=1>
    class Ingestor
    {
        protected:
        static constexpr size_t BlockSize = WindowSize / 2;

        types::Precision cd_freq_hz{};      // = 44100.0;
        types::Precision sample_period_s{}; // = 1.0 / cd_freq_hz;
        utils::FFTHelper<WindowSize> chirp{};
        utils::DerivativeHelper<WindowSize, 2> derivative_helper{};
        std::array<utils::FFTHelper<WindowSize>, Nchannels> signals{};
        std::array<utils::ExtremmaFinder, Nchannels> extremma_helpers_{};

        public:

        typedef utils::ExtremmaFinder::vector_extremma vector_extremma;
        typedef std::pair<types::Precision, types::Precision> time_bounds;

        template <size_t channel_index=0>
        utils::ExtremmaFinder & extremma_helper()
        {
            return extremma_helpers_[channel_index];
        }

        time_bounds tau_bounds_s()
        {
            return time_bounds_s(0);
        }

        time_bounds time_bounds_s(types::Precision tau_to_time_offset_s)
        {
            return std::make_pair(types::Precision(-0.5) * static_cast<int>(BlockSize) * sample_period_s + tau_to_time_offset_s, 
                                  types::Precision(+0.5) * static_cast<int>(BlockSize) * sample_period_s + tau_to_time_offset_s);
        }

        void reset(const typename utils::FFTHelper<WindowSize>::RealsT &chirp_input, types::Precision cd_freq_hz_in)
        {
            cd_freq_hz = cd_freq_hz_in;
            sample_period_s = 1.0 / cd_freq_hz;

            chirp.reset();
            chirp.input = chirp_input;
            chirp.transform();
            // chirp.conjugate();

            for (size_t channel_index = 0; channel_index < Nchannels; ++channel_index)
            {
                signals[channel_index].reset();
            }

            derivative_helper.setup(chirp.coeffs, cd_freq_hz);
        }

        // template <size_t derivative_order, typename TauT>
        // auto correlate_and_derive(TauT tau)
        // {
        //     // auto window_period_s = WindowSize * sample_period_s;
        //     return derivative_helper.template correlate_and_derive<derivative_order>(signal.coeffs, tau);
        // }

        template <typename dataInT>
        size_t run(size_t channel_index, dataInT *src, types::Precision tau_to_time_offset_s, size_t n_extremma, bool reset_heap = true)
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

            derivative_helper.template correlate_via_dft<0>(signal.coeffs);
            derivative_helper.template correlate_via_dft<1>(signal.coeffs);

            // implement a search
            auto [tau_s_lower_bound, tau_s_upper_bound] = tau_bounds_s();
            auto tau_s_step = sample_period_s;
            extremma_helper.resize(n_extremma);
            if (reset_heap)
            {
                extremma_helper.reset(n_extremma);
            }
            return extremma_helper.find_extremma(derivative_helper, signal.coeffs, tau_s_lower_bound, tau_s_upper_bound, tau_s_step, tau_to_time_offset_s);
        }
    };

    template <size_t WindowSize, size_t Nchannels=1>
    class SignalAcquirer : public Ingestor<WindowSize, Nchannels>
    {
        public:

        static constexpr size_t nDetsForHypothesis = 3;

        template <typename dataInT, typename CallbackT>
        size_t run(size_t channel_index, dataInT *src, const CallbackT & callback, types::Precision tau_to_time_offset_s, size_t n_extremma, types::Precision sync_period_s, types::Precision sync_half_gate_s, types::Precision nearby_peak_tolerance_s)
        {
            if (channel_index >= Nchannels)
            {
                return 0;
            }

            auto & extremma_helper = this->extremma_helpers_[channel_index];

            [[maybe_unused]]
            auto n_extremma_added = Ingestor<WindowSize, Nchannels>::run(channel_index, src, tau_to_time_offset_s, n_extremma, false);

            const auto & extremma = extremma_helper.extremma();

            // prune dets that are way too old
            auto prune_time_s = tau_to_time_offset_s - nDetsForHypothesis * (sync_period_s + sync_half_gate_s);
            if (!std::isnan(prune_time_s))
            {
                extremma_helper.prune_before(prune_time_s);
            }
            if (!std::isnan(nearby_peak_tolerance_s))
            {
                extremma_helper.filter_redundant(nearby_peak_tolerance_s);
            }

            if (0 == n_extremma_added)
            {
                return n_extremma_added;
            }
            
            fire_on_new_peak(channel_index, extremma, callback, tau_to_time_offset_s, sync_period_s, sync_half_gate_s);

            return n_extremma_added;
        }

        template <typename CallbackT>
        void fire_on_new_peak(size_t channel_index, const typename Ingestor<WindowSize, Nchannels>::vector_extremma & extremma, const CallbackT & callback, types::Precision tau_to_time_offset_s, types::Precision sync_period_s, types::Precision sync_half_gate_s)
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
            auto [next_youngest_time_s_lower_bound, next_youngest_time_s_upper_bound] = this->time_bounds_s(tau_to_time_offset_s);
            auto youngest_det_time_s = types::Precision{};
            size_t hypothesis_size = 0;

            auto update_time_and_gate = [&](size_t i)
            {
                hypothesis[hypothesis_size++] = i;
                extremma_mask[i] = true;

                youngest_det_time_s = extremma[i].time_s;
                next_youngest_time_s_lower_bound = youngest_det_time_s - sync_period_s - sync_half_gate_s;
                next_youngest_time_s_upper_bound = youngest_det_time_s - sync_period_s + sync_half_gate_s;
            };

            auto tau_s_step = this->sample_period_s;
            for (size_t i = 0; i < extremma.size(); ++i) 
            {
                auto det_time_s = extremma[i].time_s;
                if (det_time_s >= next_youngest_time_s_lower_bound - tau_s_step) // detection peak could be between blocks
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
                auto det_time_s = det.time_s;
                if (det_time_s >= next_youngest_time_s_lower_bound && det_time_s <= next_youngest_time_s_upper_bound)
                {
                    update_time_and_gate(i);
                    i = -1; // will be incremented back to 0
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
