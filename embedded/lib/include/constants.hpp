#pragma once

#include <numbers>

#include "types.hpp"

template <typename Precision, typename Complex = std::complex<Precision>>
struct Constants
{
static constexpr Precision pi{std::numbers::pi_v<Precision>}; // 3.14159265358979323846;
static constexpr Precision twopi{2.0 * pi};
static constexpr Complex j{0.0, 1.0};
static constexpr Complex twopij{0.0, twopi};
static constexpr Complex pij{0.0, pi};

static constexpr Precision speed_of_sound_mps{343.0};                        // meters per second
static constexpr Precision speed_of_sound_mmps{speed_of_sound_mps * 1000.0}; // millimeters per second
static constexpr Precision speed_of_sound_spm{1.0 / speed_of_sound_mps};     // seconds per meter
static constexpr Precision speed_of_sound_spmm{1.0 / speed_of_sound_mmps};   // seconds per millimeter
static constexpr Precision in_to_m{0.0254}; // inches to meters
static constexpr Precision in_to_mm{in_to_m * 1000.0}; // inches to millimeters
};