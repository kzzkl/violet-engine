#include "bezier.hpp"
#include "math/vector.hpp"

namespace violet::sample
{
float bezier::evaluate(float x, float precision) const noexcept
{
    float begin = 0.0f;
    float end = 1.0f;
    float temp = (begin + end) * 0.5f;

    float p = sample_x(temp);

    while (std::abs(p - x) > precision)
    {
        if (p > x)
        {
            end = temp;
        }
        else
        {
            begin = temp;
        }

        temp = (begin + end) * 0.5f;
        p = sample_x(temp);
    }

    return sample_y(temp);
}

vec2f bezier::sample(float t) const noexcept
{
    vec4f_simd t0 = math::load(vec4f{0.0f, 0.0f, 0.0f, 0.0f});
    vec4f_simd t1 = math::load(p1);
    vec4f_simd t2 = math::load(p2);
    vec4f_simd t3 = math::load(vec4f{1.0f, 1.0f, 0.0f, 0.0f});

    vec4f_simd t01 = vector::lerp(t0, t1, t);
    vec4f_simd t12 = vector::lerp(t1, t2, t);
    vec4f_simd t23 = vector::lerp(t2, t3, t);

    vec4f_simd t012 = vector::lerp(t01, t12, t);
    vec4f_simd t123 = vector::lerp(t12, t23, t);

    vec2f result;
    math::store(vector::lerp(t012, t123, t), result);
    return result;
}

float bezier::sample_x(float t) const noexcept
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;

    return t3 + 3 * t2 * it * p2[0] + 3 * t * it2 * p1[0];
}

float bezier::sample_y(float t) const noexcept
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float it = 1.0f - t;
    const float it2 = it * it;

    return t3 + 3 * t2 * it * p2[1] + 3 * t * it2 * p1[1];
}
} // namespace violet::sample