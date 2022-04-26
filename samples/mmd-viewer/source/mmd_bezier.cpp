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

    float p = sample_x(temp);

    while (std::abs(p - x) > precision)
    {
        if (p > x)
            end = temp;
        else
            begin = temp;

        temp = (begin + end) * 0.5f;
        p = sample_x(temp);
    }

    return sample_y(temp);
}

math::float2 mmd_bezier::sample(float t) const noexcept
{
    static constexpr math::float2 p0 = {0.0f, 0.0f};
    static constexpr math::float2 p3 = {1.0f, 1.0f};

    math::float2 p01 = math::vector_plain::lerp(p0, m_p1, t);
    math::float2 p12 = math::vector_plain::lerp(m_p1, m_p2, t);
    math::float2 p23 = math::vector_plain::lerp(m_p2, p3, t);

    math::float2 p012 = math::vector_plain::lerp(p01, p12, t);
    math::float2 p123 = math::vector_plain::lerp(p12, p23, t);

    return math::vector_plain::lerp(p012, p123, t);
}

float mmd_bezier::sample_x(float t) const noexcept
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;

    return t3 + 3 * t2 * it * m_p2[0] + 3 * t * it2 * m_p1[0];
}

float mmd_bezier::sample_y(float t) const noexcept
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;

    return t3 + 3 * t2 * it * m_p2[1] + 3 * t * it2 * m_p1[1];
}

void mmd_bezier::set(const math::float2& p1, const math::float2& p2) noexcept
{
    m_p1 = p1;
    m_p2 = p2;
}
} // namespace ash::sample::mmd