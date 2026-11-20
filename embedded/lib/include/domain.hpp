#pragma once

#include "types.hpp"
#include "constants.hpp"

template <typename Precision>
struct Domain
{
    constexpr static size_t BlockSize = 128;
    constexpr static size_t WindowSize = BlockSize * 2;

    constexpr static Precision cd_freq_hz{44100.0}; //44117.64706; // 44100.0
    constexpr static Precision block_period_s{BlockSize / cd_freq_hz};
    constexpr static Precision window_period_s{WindowSize / cd_freq_hz};
    constexpr static Precision sample_period_s{1.0 / cd_freq_hz};
};



