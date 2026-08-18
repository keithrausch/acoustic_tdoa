#ifndef FFT_HPP
#define FFT_HPP


#include <limits>
#include <cmath>

#include "types.hpp"

namespace utils
{

    static constexpr size_t Nsamples_to_Ncoeffs(size_t Nsamples)
    {
        return Nsamples / 2 + 1;
    }

    static constexpr size_t Ncoeffs_to_Nsamples(size_t Nsamples)
    {
        // must guard against Nsamples == 1
        return (Nsamples >= 2) ? (2 * (Nsamples - 1)) : 1;
    }

    static constexpr size_t get_n_bits(size_t Nsamples) {
        size_t n = Nsamples;
        size_t bits = 0;

        while (n > 1)
        {
            n >>= 1;
            ++bits;
        }

        return bits;
    };

    template <size_t Nsamples>
    class FFT_c2c_1d
    {
        // https://kovleventer.com/blog/fft_real/

        public:
        constexpr static size_t Ncoeffs = Nsamples; // complex in means no redundant data
        typedef types::array_cp<Ncoeffs> OutputT;
        typedef types::array_p<Nsamples> InputT;

        types::array_cp<Ncoeffs> twiddles;

        private:
        static constexpr size_t n_bits = get_n_bits(Nsamples);
        std::array<size_t, Nsamples> bit_reversing_table;

        public:

        FFT_c2c_1d(types::Precision exponent_sign = -1)
        {
            reset(exponent_sign);
        }

        void reset(types::Precision exponent_sign = -1.0)
        {
            static_assert((Nsamples >= 2) && (Nsamples & (Nsamples-1))==0, "FFT can only be performed on window sizes that are powers of 2");
            for (size_t k = 0; k < Ncoeffs; ++k)
            {
                twiddles[k] = std::exp( exponent_sign * constants::twopij * static_cast<types::Precision>(k) / static_cast<types::Precision>(Nsamples*2));
            }

            for (size_t i = 0; i < Ncoeffs; ++i)
            {
                bit_reversing_table[i] = reversed_index(i);
            }
        }

        void run(const types::cPrecision* input, OutputT &output)
        {
            // bit reversal of inputs for Decimation In Frequency (DIF)
            if (input != output.data())
            {
                for (size_t i = 0; i < Nsamples; ++i)
                {
                    output[bit_reversing_table[i]] = input[i];
                }
            }
            else
            {
                for (size_t i = 0; i < Nsamples; ++i)
                {
                    auto j = bit_reversing_table[i];
                    if (j > i)
                    {
                        std::swap(output[i], output[j]);
                    }
                }
            }
            
            butterfly_dit(output.data());
        }

        template <typename T>
        void rescale(T &arr)
        {
            constexpr types::Precision factor = 1.0 / Nsamples;
            for (auto & element : arr)
            {
                element *= factor;
            }
        }

        private:
        size_t reversed_index(size_t index)
        {
            constexpr size_t one = 1;
            size_t ret = 0;
            for (size_t i = 0; i < n_bits; ++i)
            {
                size_t bit = (index >> i) & one;
                ret |= bit << (n_bits-1-i);
            }
            return ret;
        }

        void butterfly_dit(types::cPrecision* buffer)
        {
            // taken from https://en.wikipedia.org/wiki/Cooley%E2%80%93Tukey_FFT_algorithm
            size_t m = 1;
            size_t exp_stride = Nsamples;
            for (size_t s = 1; s <= n_bits; ++s)
            {
                size_t mHalf = m;
                m *= 2;
                for (size_t k = 0; k < Nsamples; k+= m)
                {
                    for (size_t j = 0; j < mHalf; ++j)
                    {
                        auto omega = twiddles[exp_stride*j];
                        size_t indA = k + j;
                        size_t indB = indA + mHalf;
                        auto u = buffer[indA];
                        auto t = omega * buffer[indB];
                        buffer[indA] = u + t;
                        buffer[indB] = u - t;
                    }
                }
                exp_stride /=2;
            }
        }
    };

    template <>
    class FFT_c2c_1d<1>
    {
        public:
        constexpr static size_t Nsamples = 1;
        constexpr static size_t Ncoeffs = Nsamples; // complex in means no redundant data
        typedef types::array_cp<Ncoeffs> OutputT;
        typedef types::array_p<Nsamples> InputT;

        types::array_cp<Ncoeffs> twiddles; // necessary for r2c_1d<2>

        FFT_c2c_1d(types::Precision exponent_sign = -1)
        {
            reset(exponent_sign);
        }

        void reset(types::Precision exponent_sign = -1.0)
        {
            twiddles[0] = 1;
        }

        void run(const types::cPrecision* input, OutputT &output)
        {
            output[0] = input[0];
        }

        template <typename T>
        void rescale(T &arr)
        {
        }
    };

    template <size_t Nsamples>
    class FFT_real_1d
    {
        // https://kovleventer.com/blog/fft_real/

        public:
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT;
        typedef types::array_p<Nsamples> RealsT;
        
        private:
        typedef FFT_c2c_1d<Nsamples/2> FFT_c2c_1d_T;
        FFT_c2c_1d_T fft_c2c_1d;
        FFT_c2c_1d_T::OutputT & twiddles{fft_c2c_1d.twiddles}; // parent twiddles are identical

        public:
        FFT_real_1d(types::Precision exponent_sign = -1)
        {
            reset(exponent_sign);
        }

        void reset(types::Precision exponent_sign = -1)
        {
            // static_assert(Nsamples > 2, "cant handle Nsamples == 1 or 2 yet");
            static_assert((Nsamples >= 2) && (Nsamples & (Nsamples-1))==0, "FFT can only be performed on window sizes that are powers of 2");
            fft_c2c_1d.reset(exponent_sign);
        }

        void r2c(const RealsT &input, CoeffsT &output)
        {
            typename FFT_c2c_1d_T::OutputT& complex_results = reinterpret_cast<typename FFT_c2c_1d_T::OutputT&>(output);
            fft_c2c_1d.run(reinterpret_cast<const types::cPrecision*>(input.data()), complex_results);

            //
            // NOTE this chunk down here is a bit different than the article. i have to handle i=0 
            // first because it isnt like the others, and then i wanted to do everything in-place 
            // so i had to do writes and iterate half as much
            //output

            auto flap = [&]<bool dc_and_nyquest = false>(size_t i, size_t j)
            {
                auto a = output[i];
                auto b = output[j];
                auto b_conj = std::conj(b);

                
                auto Zx = (a + b_conj);
                auto Zy = constants::j * (b_conj - a);
                auto W_i = twiddles[i];
                output[i] = types::Precision(0.5) * (Zx + W_i*Zy);

                if constexpr(dc_and_nyquest)
                {
                    output[Ncoeffs-1] = types::Precision(0.5) * (Zx - W_i*Zy); // nyquist freq. first element in second half of outputs
                }
                
                auto W_j = twiddles[j];
                output[j] = types::Precision(0.5) * (std::conj(Zx) + W_j*std::conj(Zy));
            };

            flap.template operator()<true>(0, 0); // dc and nyquist terms
            for (size_t i = 1; i <= Ncoeffs/2; ++i)
            {
                size_t j = Ncoeffs-i-1;
                flap(i, j);
            }
            // static_assert(Nsamples % 4 == 0);

            // IF THE USER WANTS THE FULLY OUTPUT (WITH REDUNDANT COMPONENTS):
            // output[i] = 0.5 * (Zx - W*Zy)
        }

        void c2r(const CoeffsT &input, RealsT &output)
        {
            typename types::array_cp<Nsamples/2>& complex_buffer = reinterpret_cast<types::array_cp<Nsamples/2>&>(output);

            // flap.template operator()<true>(0, Ncoeffs-1); // dc and nyquist terms
            {
                constexpr size_t i = 0; 
                constexpr size_t j = Ncoeffs-1;
                auto P = input[i];
                auto Q = input[j];
                auto W = twiddles[i];
                auto Zx = /* types::Precision(0.5) * */ (P + Q);
                auto Zy = /* types::Precision(0.5) * */ W * (P - Q);
                auto Z = Zx + constants::j * Zy;
                // auto Zm_conj = Zx - constants::j * Zy;
                // auto Zm = std::conj(Zm_conj);

                complex_buffer[i] = Z;
                // complex_output[Ncoeffs-1] = Zm;
            }
            for (size_t i = 1; i <= Ncoeffs/2; ++i)
            {
                size_t j = Ncoeffs-i-1;

                auto P = input[i];
                auto Q = input[j];
                auto Q_conj = std::conj(Q); // pretend we have the redundant coeffs
                auto W = twiddles[i];

                auto Zx = /*types::Precision(0.5) * */ (P + Q_conj);
                auto Zy = /*types::Precision(0.5) * */ W * (P - Q_conj);
                
                auto Z = Zx + constants::j * Zy;
                auto Zm_conj = Zx - constants::j * Zy;
                auto Zm = std::conj(Zm_conj);

                complex_buffer[i] = Z;
                complex_buffer[j] = Zm;
            }
            // static_assert(Nsamples % 4 == 0);

            // types::array_cp<Nsamples/2> temp = complex_buffer;
            fft_c2c_1d.run(complex_buffer.data(), complex_buffer);

        }

        template <typename T>
        void rescale(T &arr)
        {
            constexpr types::Precision factor = 1.0 / Nsamples; // NOTE: not multiplying by 2
            for (auto & element : arr)
            {
                element *= factor;
            }
        }
    };

    template <>
    class FFT_real_1d<1>
    {
        // https://kovleventer.com/blog/fft_real/

        public:
        constexpr static size_t Nsamples = 1;
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT;
        typedef types::array_p<Nsamples> RealsT;

        FFT_real_1d(types::Precision exponent_sign = -1)
        {
            reset(exponent_sign);
        }

        void reset(types::Precision exponent_sign = -1)
        {}

        void r2c(const RealsT &input, CoeffsT &output)
        {
            output[0] = input[0];
        }

        void c2r(const CoeffsT &input, RealsT &output)
        {
            output[0] = input[0].real();
        }

        template <typename T>
        void rescale(T &arr)
        {}
    };

    template <>
    class FFT_real_1d<2>
    {
        // https://kovleventer.com/blog/fft_real/

        public:
        constexpr static size_t Nsamples = 2;
        constexpr static size_t Ncoeffs = Nsamples_to_Ncoeffs(Nsamples);
        typedef types::array_cp<Ncoeffs> CoeffsT;
        typedef types::array_p<Nsamples> RealsT;

        FFT_real_1d(types::Precision exponent_sign = -1)
        {
            reset(exponent_sign);
        }

        void reset(types::Precision exponent_sign = -1)
        {}

        void r2c(const RealsT &input, CoeffsT &output)
        {
            const auto x0 = input[0];
            const auto x1 = input[1];

            output[0] = x0 + x1;
            output[1] = x0 - x1;
        }

        void c2r(const CoeffsT &input, RealsT &output)
        {
            const auto dc = input[0];
            const auto nyquist = input[1];

            output[0] = /* types::Precision(0.5) * */ (dc.real() + nyquist.real());
            output[1] = /* types::Precision(0.5) * */ (dc.real() - nyquist.real());
            // NOTE: removing *0.5 because we need to scale the output by 2
        }

        template <typename T>
        void rescale(T &arr)
        {
            constexpr types::Precision factor = 1.0 / Nsamples; // NOTE: not multipyling by 2
            for (auto & element : arr)
            {
                element *= factor;
            }
        }
    };
}
#endif