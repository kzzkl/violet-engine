#pragma once

#include "math/math.hpp"

namespace violet
{
enum light_type
{
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
};

struct light_component
{
    light_type type;
    vec3f color;
};
} // namespace violet