#include "mmd_bezier.hpp"

namespace ash::sample::mmd
{
mmd_bezier::mmd_bezier(const math::float2& p1, const math::float2& p2) noexcept : m_p1(p1), m_p2(p2)
{
}

float mmd_bezier::evaluate(float x, float precision) const noexcept
{
    float begin = 0.0f;
    float end = 1.0f;
    float temp = (begin + end) * 0.5f;

    math::float2 p = sample(temp);

    while (std::abs(p[0] - x) > precision)
    {
        if (p[0] > x)
            end = temp;
        else
            begin = temp;

        temp = (begin + end) * 0.5f;
        p = sample(temp);
    }

    return p[1];
}

math::float2 mmd_bezier::sample(float t) const noexcept
{
    static const math::float2 p0 = {0.0f, 0.0f};
    static const math::float2 p3 = {1.0f, 1.0f};

    math::float2 p01 = math::vector_plain::lerp(p0, m_p1, t);
    math::float2 p12 = math::vector_plain::lerp(m_p1, m_p2, t);
    math::float2 p23 = math::vector_plain::lerp(m_p2, p3, t);

    math::float2 p012 = math::vector_plain::lerp(p01, p12, t);
    math::float2 p123 = math::vector_plain::lerp(p12, p23, t);

    return math::vector_plain::lerp(p012, p123, t);
}

void mmd_bezier::set(const math::float2& p1, const math::float2& p2) noexcept
{
    m_p1 = p1;
    m_p2 = p2;
}
} // namespace ash::sample::mmd