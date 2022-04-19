#pragma once

#include "math.hpp"

namespace ash::sample::mmd
{
class mmd_bezier
{
public:
    mmd_bezier() = default;
    mmd_bezier(const math::float2& p1, const math::float2& p2) noexcept;

    float evaluate(float x, float precision = 0.00001f) const noexcept;

    math::float2 sample(float t) const noexcept;
    float sample_x(float t) const noexcept;
    float sample_y(float t) const noexcept;

    void set(const math::float2& p1, const math::float2& p2) noexcept;

private:
    math::float2 m_p1;
    math::float2 m_p2;
};
} // namespace ash::sample::mmd