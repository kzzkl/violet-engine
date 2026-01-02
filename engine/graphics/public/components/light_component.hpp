#pragma once

#include "math/types.hpp"

namespace violet
{
enum light_type : std::uint8_t
{
    LIGHT_DIRECTIONAL,
    LIGHT_POINT,
};

struct light_component
{
    light_type type{LIGHT_DIRECTIONAL};
    vec3f color{1.0f, 1.0f, 1.0f};

    bool cast_shadow{false};
};
} // namespace violet