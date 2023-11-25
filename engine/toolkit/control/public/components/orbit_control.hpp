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

    float speed = 3.0f;

    bool dirty = true;
};
} // namespace violet