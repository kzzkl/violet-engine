#pragma once

#include "math/math.hpp"
#include "math/types.hpp"

namespace violet
{
struct orbit_control_component
{
    vec3f target = {};

    float radius = 1.0f;
    float min_radius = 0.01f;

    float theta = math::HALF_PI;
    float phi = math::HALF_PI;

    float radius_speed = 1.0f;
    float theta_speed = math::PI;
    float phi_speed = 2.0f * math::PI;
};
} // namespace violet