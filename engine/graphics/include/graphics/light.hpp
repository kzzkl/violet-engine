#pragma once

#include "math/math.hpp"

namespace ash::graphics
{
struct point_light
{
};

struct directional_light
{
    math::float3 direction;
    math::float3 color;
};
} // namespace ash::graphics