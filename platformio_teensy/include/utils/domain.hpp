#pragma once

#include "types.hpp"
#include "constants.hpp"

namespace domain
{
constexpr static size_t BlockSize = 128;
constexpr static size_t WindowSize = BlockSize * 2;
typedef std::array<uint16_t, BlockSize> Block_ui16;
typedef std::array<uint16_t, WindowSize> Window_ui16;
typedef std::array<double, WindowSize> Window_d;
typedef std::array<double, BlockSize> Block_d;


constexpr types::Precision cd_freq_hz = 44100.0; //44117.64706; // 44100.0
constexpr types::Precision block_period_s = BlockSize / cd_freq_hz;
constexpr types::Precision window_period_s = WindowSize / cd_freq_hz;
constexpr types::Precision sample_period_s = 1.0 / cd_freq_hz;
}
