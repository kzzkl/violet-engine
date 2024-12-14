#pragma once

#include "math/types.hpp"

namespace violet::sample
{
class bezier
{
public:
    bezier() = default;
    bezier(const vec2f& p1, const vec2f& p2) noexcept;

    float evaluate(float x, float precision = 0.00001f) const noexcept;

    vec2f sample(float t) const noexcept;
    float sample_x(float t) const noexcept;
    float sample_y(float t) const noexcept;

    void set(const vec2f& p1, const vec2f& p2) noexcept;

private:
    vec2f m_p1;
    vec2f m_p2;
};
} // namespace violet::sample