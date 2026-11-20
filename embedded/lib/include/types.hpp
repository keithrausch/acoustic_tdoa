#pragma once

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>

template <typename Precision, typename TimeIndex, typename CorrPeakT = Precision>
struct Types
{
    using precision_type = Precision;
    using real_type = Precision;
    using complex_type = std::complex<Precision>;

    using time_index_type = TimeIndex;
    using time_fractional_index_type = float; // no reason to make this bigger
    using corr_peak_type = CorrPeakT;

    template <size_t N>
    using array_r = std::array<real_type, N>;

    template <size_t N>
    using array_c = std::array<complex_type, N>;
};