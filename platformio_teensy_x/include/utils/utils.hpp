#ifndef DETECTOR_HPP
#define DETECTOR_HPP

#include <fftw3.h>
// #include <fftw/fftw-3.3.10/api/fftw3.h>
#include <limits>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "include/utils/constants.hpp"
#include "include/utils/types.hpp"

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

    constexpr size_t Nsamples_to_Ncoeffs(size_t Nsamples)
    {
        // return Nsamples/2+1;
        return Nsamples / 2;
    }

    constexpr size_t Ncoeffs_to_Nsamples(size_t Nsamples)
    {
        // return 2*(Nsamples-1);
        return 2 * (Nsamples);
    }

    template <size_t Ncoeffs>
    static types::Precision reconstruct_at_index(const types::array_cp<Ncoeffs> &coeffs, size_t k, size_t n_lowest_coeffs = Ncoeffs)
    {
        constexpr types::Precision Nsamples = Ncoeffs_to_Nsamples(Ncoeffs);

        types::cPrecision sum(0.0, 0.0);
        for (size_t m = 0; m < std::min(coeffs.size(), n_lowest_coeffs); ++m)
        {
            auto coeff = coeffs[m];
            sum += coeff * std::exp(constants::twopij * static_cast<types::Precision>(k * m) / Nsamples);
        }
        return sum.real();
    }

    template <size_t Nsamples, size_t max_derivative_order>
    struct DerivativeHelper
    {
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT; // elements are 2 doubles, so we need half the length
        typedef types::array_p<Nsamples> RealsT;

        std::array<types::array_p<Ncoeffs>, max_derivative_order + 1> freqs_to_power;

        void fill_frequencies(types::Precision sample_period)
        {
            for (size_t i = 0; i < freqs_to_power[0].size(); ++i)
            {
                types::Precision freq = i * sample_period / static_cast<types::Precision>(Nsamples);
                freqs_to_power[0][i] = 1.0;
                for (size_t o = 1; o <= max_derivative_order; ++o)
                {
                    freqs_to_power[o][i] = freqs_to_power[o - 1][i] * freq;
                }
            }
        }

        template <size_t derivative_order, bool real_only, bool coeffA_already_conjugated, bool fast_derivative=false, typename TauT>
        auto correlate_impl(const CoeffsT &A, const CoeffsT &B, types::Precision duration_s, const TauT tau)
        {
            types::cPrecision sum(0.0, 0.0);
            size_t N = std::min(A.size(), B.size());
            for (size_t i = 0; i < N; ++i)
            {
                auto a = A[i];
                auto b = B[i];

                types::cPrecision term_i = b * std::exp(constants::twopij * freqs_to_power[1][i] * tau);
                if constexpr (coeffA_already_conjugated)
                {
                    term_i *= a;
                }
                else
                {
                    term_i *= std::conj(a);
                }

                if constexpr (derivative_order > 0)
                {
                    term_i *= freqs_to_power[derivative_order][i];
                }

                sum += term_i;
            }

            if constexpr((1 == derivative_order) && fast_derivative && real_only)
            {
                return sum.imag();
            }

            if constexpr (derivative_order > 0)
            {
                sum *= utils::pow<derivative_order, types::cPrecision>(constants::twopij);
            }

            sum *= duration_s;

            if constexpr (real_only)
            {
                return sum.real();
            }
            else
            {
                return sum;
            }
        }

        template <bool real_only, bool coeffA_already_conjugated, typename TauT, size_t... indicesT>
        auto correlate_and_derive_impl(const CoeffsT &A, const CoeffsT &B, types::Precision duration_s, const TauT tau, std::integer_sequence<size_t, indicesT...>)
        {
            typedef decltype(correlate_impl<0, real_only, coeffA_already_conjugated>(A, B, duration_s, tau)) correlation_return_type;
            std::array<correlation_return_type, sizeof...(indicesT)> ret;
            ((ret[indicesT] = correlate_impl<indicesT, real_only, coeffA_already_conjugated>(A, B, duration_s, tau)), ...);
            return ret;
        }

        template <size_t derivative_order, bool coeffA_already_conjugated, bool real_only = false, typename TauT = types::Precision>
        auto correlate_and_derive(const CoeffsT &A, const CoeffsT &B, types::Precision duration_s, const TauT tau)
        {
            return correlate_and_derive_impl<real_only, coeffA_already_conjugated>(A, B, duration_s, tau, typename std::make_index_sequence<derivative_order + 1>());
        }

        template <typename TauT>
        auto eval_and_print(const CoeffsT &A, const CoeffsT &B, types::Precision duration_s, const TauT tau)
        {
            auto [f, f_d1, f_d2] = correlate_and_derive<2, true>(A, B, duration_s, tau);

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
        }
    };

    template <size_t Nsamples>
    struct FFTHelper
    {
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT; // elements are 2 doubles, so we need half the length
        typedef types::array_p<Nsamples> RealsT;
        fftw_plan plan{};

        RealsT input;
        types::array_cp<Ncoeffs + 1> coeffs_buffer; // fftw needs 1 extra element when executing
        CoeffsT &coeffs{reinterpret_cast<CoeffsT &>(coeffs_buffer)};

        // std::array<double, Nsamples> input_buffer;
        // std::array<std::complex<double>, Ncoeffs+1> coeffs_buffer;
        // CoeffsT coeffs;

        types::Precision coeff_norm_thresh{1E-4};
        size_t n_lowest{Ncoeffs};

        ~FFTHelper()
        {
            fftw_destroy_plan(plan);
        }

        // FFTHelper& operator=(FFTHelper&& rhs) = default;
        FFTHelper() = default;
        FFTHelper(FFTHelper &&rhs) = default;

        void reset()
        {
            input.fill(0.0);
            coeffs.fill(types::cPrecision(0.0, 0.0));

            fftw_destroy_plan(plan);

            // plan = fftw_plan_dft_r2c_1d(Nsamples, input_buffer.data(), reinterpret_cast<fftw_complex*>(coeffs_buffer.data()), FFTW_ESTIMATE);

            static_assert(std::is_same<types::Precision, double>::value, "YOU CHANGED THE types::Precision. YOU NOW NEED TO CONVERT BETWEEN DOUBLE AND WHATEVER YOUR types::Precision IS. SEE THE COMMENTED CODE.");
            plan = fftw_plan_dft_r2c_1d(Nsamples, input.data(), reinterpret_cast<fftw_complex *>(coeffs.data()), FFTW_ESTIMATE);

            n_lowest = coeffs.size();
        }

        void transform_and_normalize_coefficients()
        {
            // for (size_t i = 0; i < Nsamples; ++i)
            // {
            //     input_buffer[i] = input[i];
            // }
            fftw_execute(plan);
            // for (size_t i = 0; i < Ncoeffs; ++i)
            // {
            //     coeffs[i] = coeffs_buffer[i];
            // }

            coeffs[0] /= static_cast<types::Precision>(Nsamples); // dont mult by 2. there's only 1 DC term
            for (size_t i = 1; i < coeffs.size(); ++i)
            {
                coeffs[i] /= static_cast<types::Precision>(Nsamples); // normalize
                coeffs[i] *= 2.0;                              // make up for not having half the coefficients
            }
        }

        // void discard_high_freq_coefficients()
        // {

        //     n_lowest = coeffs.size();
        //     for (int i = coeffs.size() - 1; i >= 0; --i)
        //     {
        //         types::Precision mag_squared = std::norm(coeffs[i]);
        //         if (mag_squared >= coeff_norm_thresh * coeff_norm_thresh)
        //         {
        //             // n_lowest was set from previous iteration
        //             break;
        //         }

        //         n_lowest = i;
        //     }
        // }

        void conjugate()
        {
            for (auto &element : coeffs)
            {
                element = std::conj(element);
            }
        }

        static FFTHelper construct_simple(types::Precision sample_period_s, const types::SoundFunctionT &chirp_func, const WaveParams &chirp_params, bool conjugate=false)
        {
            utils::FFTHelper<Nsamples> chirp;
            chirp.reset();
            chirp.input = utils::create_template<Nsamples>(0, sample_period_s, chirp_func, chirp_params);
            chirp.transform_and_normalize_coefficients();
            
            if (conjugate)
            {
                chirp.conjugate();
            }

            // pulse.discard_high_freq_coefficients();
            // print("input:", sample);

            return chirp;
        }
    };


    template <typename CallableT>
    std::pair<types::Precision, types::Precision> newton(const CallableT &fd0_fd1_fd2, double guess_tau_s, size_t n_steps = 10)
    {
        types::Precision x_n = guess_tau_s; // 0 + 0.5 * sample_period_s;
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

        template <typename CallableFD0T, typename CallableFD1T, typename CallableFD0FD1FD2T>
        size_t find_extremma(const CallableFD0T &fd0, const CallableFD1T &fd1, const CallableFD0FD1FD2T &fd0_fd1_fd2, types::Precision tau_s_lower_bound, types::Precision tau_s_upper_bound, types::Precision tau_s_step, types::Precision tau_to_time_offset_s=std::numeric_limits<types::Precision>::quiet_NaN())
        {
            bool offset_provided = !std::isnan(tau_to_time_offset_s);
            if (!offset_provided)
            {
                tau_to_time_offset_s = 0.0;
            }

            auto sign = [](types::Precision v)
            {
                return (v > 0.0) ? +1 : ((v < 0.0) ? -1 : 0);
            };

            for (auto tau_s = tau_s_lower_bound; tau_s < tau_s_upper_bound; tau_s += tau_s_step)
            {
                auto f_d1 = fd1(tau_s);
                auto f_d1_sign = sign(f_d1);

                if (f_d1_sign != last_f_d1_sign)
                {
                    auto f_d0_abs = std::fabs(fd0(tau_s));
                    record_extremma(CandidateExtremma{.time_s = tau_s+tau_to_time_offset_s, .value = f_d0_abs/*, .id=count++*/});
                }

                last_f_d1_sign = f_d1_sign;
            }

            // for (auto tau_s = tau_s_lower_bound; tau_s < tau_s_upper_bound; tau_s += tau_s_step)
            // {
            //     auto [f_d0, f_d1] = fd0_fd1(tau_s);
            //     auto f_d0_abs = std::fabs(f_d0);
            //     auto f_d1_sign = sign(f_d1);

            //     if (f_d1_sign != last_f_d1_sign)
            //     {
            //         // the derivative has changed signs, but is this a better point or was the previous one
            //         if (last_f_d0_abs > f_d0_abs)
            //         {
            //             record_extremma(CandidateExtremma{.time_s = last_tau_s+tau_to_time_offset_s, .value = last_f_d0_abs/*, .id=count++*/});
            //         }
            //         else
            //         {
            //             record_extremma(CandidateExtremma{.time_s = tau_s+tau_to_time_offset_s, .value = f_d0_abs/*, .id=count++*/});
            //         }
            //     }

            //     last_f_d0_abs = f_d0_abs;
            //     last_f_d1_sign = f_d1_sign;
            //     last_tau_s = tau_s;
            // }

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

        void print(types::Precision sample_period_s, types::Precision subtract_this=0.0)
        {
            for (size_t i = 0; i < extremma_.size(); ++i)
            {
                auto &[time_s, value/*, id*/] = extremma_[i];
                std::cout << "peak " << i << ") ";

                {
                    std::stringstream ss;
                    ss << "tau_index: ";
                    ss << std::fixed << std::showpoint << std::showpos;
                    ss << std::setprecision(6);
                    // ss << std::setw(10) << std::setfill('0');
                    ss << (time_s-subtract_this)/sample_period_s << "[] ";
                    ss << std::setprecision(8);
                    ss << "time_s: " << time_s << "s";
                    std::cout << ss.str();
                }

                {
                    std::stringstream ss;
                    ss << std::scientific << std::showpos;
                    ss << std::setprecision(8);
                    ss << " value:" << value;
                    std::cout << ss.str();
                }

                // {
                //     std::stringstream ss;
                //     // ss << std::scientific << std::showpos;
                //     // ss << std::setprecision(8);
                //     ss << " id:" << id << "\n";
                //     std::cout << ss.str();
                // }
            }
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

    template <size_t WindowSize>
    class Ingestor
    {
        protected:
        static constexpr size_t BlockSize = WindowSize / 2;

        types::Precision cd_freq_hz{};      // = 44100.0;
        types::Precision sample_period_s{}; // = 1.0 / cd_freq_hz;
        utils::FFTHelper<WindowSize> chirp{};
        utils::FFTHelper<WindowSize> signal{};
        utils::DerivativeHelper<WindowSize, 2> derivative_helper{};
        utils::ExtremmaFinder extremma_helper_{};

        public:

        typedef utils::ExtremmaFinder::vector_extremma vector_extremma;
        typedef std::pair<types::Precision, types::Precision> time_bounds;

        utils::ExtremmaFinder & extremma_helper()
        {
            return extremma_helper_;
        }

        time_bounds tau_bounds_s()
        {
            return time_bounds_s(0);
        }

        time_bounds time_bounds_s(types::Precision tau_to_time_offset_s)
        {
            return std::make_pair(-0.5 * static_cast<int>(BlockSize) * sample_period_s + tau_to_time_offset_s, 
                                  +0.5 * static_cast<int>(BlockSize) * sample_period_s + tau_to_time_offset_s);
        }

        void reset(const typename utils::FFTHelper<WindowSize>::RealsT &chirp_input, types::Precision cd_freq_hz_in)
        {
            cd_freq_hz = cd_freq_hz_in;
            sample_period_s = 1.0 / cd_freq_hz;

            chirp.reset();
            chirp.input = chirp_input;
            chirp.transform_and_normalize_coefficients();
            chirp.conjugate();

            signal.reset();

            derivative_helper.fill_frequencies(cd_freq_hz);
        }

        template <size_t derivative_order, typename TauT>
        auto correlate_and_derive(TauT tau)
        {
            constexpr bool real_only = true;
            constexpr bool coeffA_already_conjugated = true;
            auto window_period_s = WindowSize * sample_period_s;
            return derivative_helper.template correlate_and_derive<derivative_order, real_only, coeffA_already_conjugated>(chirp.coeffs, signal.coeffs, window_period_s, tau);
        }

        template <typename dataInT>
        size_t run(dataInT *src, types::Precision tau_to_time_offset_s, size_t n_extremma, bool reset_heap = true)
        {
            if (!src)
            {
                return 0;
            }

            // roll data
            for (size_t i = 0; i < BlockSize; ++i)
            {
                signal.input[i] = signal.input[i + BlockSize];
                signal.input[i + BlockSize] = src[i];
            }

            signal.transform_and_normalize_coefficients();

            auto fd0 = [this](auto tau) { return correlate_and_derive<0>(tau)[0]; };
            auto fd1 = [this](auto tau) // { return correlate_and_derive<1>(tau); };
            {
                constexpr size_t derivative_order = 1;
                constexpr bool real_only = true;
                constexpr bool coeffA_already_conjugated = true;
                constexpr types::Precision window_period_s = 0.0;
                constexpr bool fast_derivative = true;
                return derivative_helper.template correlate_impl<derivative_order, real_only, coeffA_already_conjugated, fast_derivative, types::Precision>(chirp.coeffs, signal.coeffs, window_period_s, tau);
                // auto window_period_s = WindowSize * sample_period_s;
                // return derivative_helper.template correlate_and_derive<derivative_order, real_only, coeffA_already_conjugated>(chirp.coeffs, signal.coeffs, window_period_s, tau);
            };
            auto fd0_fd1_fd2 = [this](auto tau) { return correlate_and_derive<2>(tau); };

            // implement a search
            auto [tau_s_lower_bound, tau_s_upper_bound] = tau_bounds_s();
            auto tau_s_step = sample_period_s;
            extremma_helper_.resize(n_extremma);
            if (reset_heap)
            {
                extremma_helper_.reset(n_extremma);
            }
            return extremma_helper_.find_extremma(fd0, fd1, fd0_fd1_fd2, tau_s_lower_bound, tau_s_upper_bound, tau_s_step, tau_to_time_offset_s);
        }
    };

    template <size_t WindowSize>
    class SignalAcquirer : public Ingestor<WindowSize>
    {
        public:

        static constexpr size_t nDetsForHypothesis = 3;
        template <typename dataInT, typename CallbackT>
        void run(dataInT *src, const CallbackT & callback, types::Precision tau_to_time_offset_s, size_t n_extremma, types::Precision sync_period_s, types::Precision sync_half_gate_s, types::Precision nearby_peak_tolerance_s)
        {
            [[maybe_unused]]
            auto n_extremma_added = Ingestor<WindowSize>:: template run(src, tau_to_time_offset_s, n_extremma, false);

            const auto & extremma = this->extremma_helper_.extremma();

            // prune dets that are way too old
            auto prune_time_s = tau_to_time_offset_s - nDetsForHypothesis * (sync_period_s + sync_half_gate_s);
            if (!std::isnan(prune_time_s))
            {
                this->extremma_helper_.prune_before(prune_time_s);
            }
            if (!std::isnan(nearby_peak_tolerance_s))
            {
                this->extremma_helper_.filter_redundant(nearby_peak_tolerance_s);
            }

            if (0 == n_extremma_added)
            {
                return;
            }

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

            for (int i = 0; i < extremma.size(); ++i)
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
                callback(extremma[hypothesis[0]]);
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
}

#endif
