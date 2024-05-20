#pragma once

#include "math/math.hpp"

namespace violet
{
struct orbit_control
{
    float3 target = {};

    float r = 1.0f;
    float theta = PI_PIDIV2;
    float phi = PI_PIDIV2;

    float r_speed = 1.0f;
    float theta_speed = PI;
    float phi_speed = 2.0f * PI;

    bool dirty = true;
};
} // namespace violet