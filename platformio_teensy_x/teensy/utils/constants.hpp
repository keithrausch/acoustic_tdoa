#pragma once

#include <numbers>

#include "types.hpp"

namespace constants
{
static constexpr types::Precision pi = std::numbers::pi_v<types::Precision>;
static constexpr types::Precision twopi = 2.0 * pi;
static constexpr types::cPrecision j(0.0, 1.0);
static constexpr types::cPrecision twopij(0.0, twopi);
static constexpr types::cPrecision pij(0.0, pi);

static constexpr types::Precision speed_of_sound_mps = 343.0;                        // meters per second
static constexpr types::Precision speed_of_sound_mmps = speed_of_sound_mps * 1000.0; // millimeters per second
static constexpr types::Precision in_to_m = 0.0254; // inches to meters
static constexpr types::Precision in_to_mm = in_to_m * 1000.0; // inches to millimeters
}