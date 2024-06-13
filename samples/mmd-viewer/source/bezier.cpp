#include "bezier.hpp"

namespace violet::sample
{
bezier::bezier(const float2& p1, const float2& p2) noexcept : m_p1(p1), m_p2(p2)
{
}

float bezier::evaluate(float x, float precision) const noexcept
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

float2 bezier::sample(float t) const noexcept
{
    vector4 p0 = vector::set(0.0f, 0.0f, 0.0f, 0.0f);
    vector4 p1 = vector::load(m_p1);
    vector4 p2 = vector::load(m_p2);
    vector4 p3 = vector::set(1.0f, 1.0f, 0.0f, 0.0f);

    vector4 p01 = vector::lerp(p0, p1, t);
    vector4 p12 = vector::lerp(p1, p2, t);
    vector4 p23 = vector::lerp(p2, p3, t);

    vector4 p012 = vector::lerp(p01, p12, t);
    vector4 p123 = vector::lerp(p12, p23, t);

    return vector::store<float2>(vector::lerp(p012, p123, t));
}

float bezier::sample_x(float t) const noexcept
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;

    return t3 + 3 * t2 * it * m_p2[0] + 3 * t * it2 * m_p1[0];
}

float bezier::sample_y(float t) const noexcept
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;

    return t3 + 3 * t2 * it * m_p2[1] + 3 * t * it2 * m_p1[1];
}

void bezier::set(const float2& p1, const float2& p2) noexcept
{
    m_p1 = p1;
    m_p2 = p2;
}
} // namespace violet::sample