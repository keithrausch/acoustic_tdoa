
#ifndef SPECIFICS_HPP
#define SPECIFICS_HPP

#include "utils.hpp"
#include "domain.hpp"
#include "types.hpp"

constexpr size_t n_blocks_for_chirp_cycle = 344/2;
constexpr uint32_t chirp_period_us = n_blocks_for_chirp_cycle*domain::block_period_s*1E6;

// chirp
static constexpr auto chirp_func = utils::sinc<types::Precision>; // utils::sinc2<types::Precision>;
constexpr utils::WaveParams chirp_params = utils::WaveParams{.amplitude = 30000.0, .center_s = domain::window_period_s * 0.5, .freq_hz = 5E3};
// constexpr utils::FFTHelper<domain::WindowSize> chirp = utils::FFTHelper<domain::WindowSize>::construct_simple(domain::sample_period_s, chirp_func, chirp_params);

#endif