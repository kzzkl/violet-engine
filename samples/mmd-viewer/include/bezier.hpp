#pragma once

#include "math/types.hpp"

namespace violet
{
struct bezier
{
    float evaluate(float x, float precision = 0.00001f) const noexcept;

    vec2f sample(float t) const noexcept;
    float sample_x(float t) const noexcept;
    float sample_y(float t) const noexcept;

    vec2f p1;
    vec2f p2;
};
} // namespace violet