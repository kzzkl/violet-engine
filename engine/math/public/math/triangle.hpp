#pragma once

#include "math/vector.hpp"

namespace violet
{
struct triangle
{
    static float get_distance(
        const vec3f& a,
        const vec3f& b,
        const vec3f& c,
        const vec3f& p) noexcept
    {
        return std::sqrt(get_distance_sq(a, b, c, p));
    }

    static float get_distance_sq(
        const vec3f& a,
        const vec3f& b,
        const vec3f& c,
        const vec3f& p) noexcept
    {
        vec3f closest_point = get_closest_point(a, b, c, p);
        return vector::length_sq(closest_point - p);
    }

    static vec3f get_closest_point(
        const vec3f& a,
        const vec3f& b,
        const vec3f& c,
        const vec3f& p) noexcept
    {
        const vec3 ab = b - a;
        const vec3 ac = c - a;
        const vec3 ap = p - a;

        const float d1 = vector::dot(ab, ap);
        const float d2 = vector::dot(ac, ap);

        // Vertex region A
        if (d1 <= 0.0f && d2 <= 0.0f)
        {
            return a;
        }

        const vec3 bp = p - b;

        const float d3 = vector::dot(ab, bp);
        const float d4 = vector::dot(ac, bp);

        // Vertex region B
        if (d3 >= 0.0f && d4 <= d3)
        {
            return b;
        }

        const float vc = (d1 * d4) - (d3 * d2);

        // Edge region AB
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            const float v = d1 / (d1 - d3);
            return a + ab * v;
        }

        const vec3 cp = p - c;

        const float d5 = vector::dot(ab, cp);
        const float d6 = vector::dot(ac, cp);

        // Vertex region C
        if (d6 >= 0.0f && d5 <= d6)
        {
            return c;
        }

        const float vb = (d5 * d2) - (d1 * d6);

        // Edge region AC
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            const float w = d2 / (d2 - d6);
            return a + ac * w;
        }

        const float va = (d3 * d6) - (d5 * d4);

        // Edge region BC
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            const vec3 bc = c - b;

            const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));

            return b + bc * w;
        }

        // Face region
        const float denom = 1.0f / (va + vb + vc);

        const float v = vb * denom;
        const float w = vc * denom;

        return a + ab * v + ac * w;
    }
};
} // namespace violet