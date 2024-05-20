#pragma once

#include "math/math.hpp"

namespace violet
{
enum light_type
{
    LIGHT_TYPE_DIRECTIONAL,
    LIGHT_TYPE_POINT,
};

struct light
{
    light_type type;
    float3 color;
};
} // namespace violet