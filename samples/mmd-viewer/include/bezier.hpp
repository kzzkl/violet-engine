#pragma once

#include "math/math.hpp"

namespace violet::sample
{
class bezier
{
public:
    bezier() = default;
    bezier(const float2& p1, const float2& p2) noexcept;

    float evaluate(float x, float precision = 0.00001f) const noexcept;

    float2 sample(float t) const noexcept;
    float sample_x(float t) const noexcept;
    float sample_y(float t) const noexcept;

    void set(const float2& p1, const float2& p2) noexcept;

private:
    float2 m_p1;
    float2 m_p2;
};
} // namespace violet::sample::mmd