
#ifndef SPECIFICS_HPP
#define SPECIFICS_HPP

#include "utils.hpp"
#include "domain.hpp"
#include "types.hpp"

using Precision = double;
using TimeIndex = uint32_t;
using types = Types<Precision, TimeIndex, float>;
using domain = Domain<types::precision_type>;
using constants = Constants<types::precision_type, types::complex_type>;

constexpr size_t n_blocks_for_chirp_cycle = 344/2;
constexpr uint32_t chirp_period_us = n_blocks_for_chirp_cycle*domain::block_period_s*1E6;

// chirp
static constexpr auto chirp_func = utils::sinc<Precision>; // utils::sinc2<types::Precision>;
constexpr utils::WaveParams chirp_params = utils::WaveParams{.amplitude = 30000.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 5E3};

#endif