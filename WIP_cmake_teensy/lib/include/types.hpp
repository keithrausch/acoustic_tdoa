#pragma once

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace types
{

    typedef double Precision;
    typedef std::complex<Precision> cPrecision;

    template <size_t N>
    using array_p = std::array<Precision, N>;

    template <size_t N>
    using array_cp = std::array<cPrecision, N>;

    // typedef std::function<types::Precision(types::Precision)> SoundFunctionT;
}