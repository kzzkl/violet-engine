#pragma once

#include "math/math.hpp"

namespace violet
{
struct orbit_control
{
    float3 target = {};

    float r = 1.0f;
    float theta = math::HALF_PI;
    float phi = math::HALF_PI;

    float r_speed = 1.0f;
    float theta_speed = math::PI;
    float phi_speed = 2.0f * math::PI;
};
} // namespace violet