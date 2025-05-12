#pragma once

#include "math/matrix.hpp"
#include "math/vector.hpp"
#include <span>

namespace violet
{
template <typename T>
struct sphere3
{
    using value_type = T;

    vec3<T> center;
    T radius{-1};

    [[nodiscard]] bool operator==(const sphere3& other) const noexcept
    {
        return center == other.center && radius == other.radius;
    }

    [[nodiscard]] bool operator!=(const sphere3& other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] operator bool() const noexcept
    {
        return radius >= 0.0f;
    }

    operator vec4<T>() const noexcept
    {
        return {center.x, center.y, center.z, radius};
    }
};

using sphere3f = sphere3<float>;

struct sphere
{
    template <typename T>
    static inline void expand(sphere3<T>& sphere, const vec3<T>& point) noexcept
    {
        if (!sphere)
        {
            sphere.center = point;
            sphere.radius = 0.0f;
            return;
        }

        float distance = vector::length(point - sphere.center);
        sphere.radius = std::max(sphere.radius, distance);
    }

    template <typename T>
    static inline void expand(sphere3<T>& sphere, std::span<const vec3<T>> points) noexcept
    {
        if (points.empty())
        {
            return;
        }

        if (!sphere)
        {
            sphere.center = points[0];
        }

        float distance_sq = 0.0f;
        for (const auto& point : points)
        {
            distance_sq = std::max(distance_sq, vector::length_sq(point - sphere.center));
        }

        sphere.radius = std::max(sphere.radius, std::sqrt(distance_sq));
    }

    template <typename T>
    static inline void expand(sphere3<T>& sphere, const sphere3<T>& other) noexcept
    {
        if (!sphere)
        {
            sphere = other;
            return;
        }

        if (sphere.center == other.center)
        {
            sphere.radius = std::max(sphere.radius, other.radius);
            return;
        }

        vec3f direction = vector::normalize(other.center - sphere.center);

        vec3f p0 = other.center + direction * other.radius;
        vec3f p1 = sphere.center - direction * sphere.radius;

        sphere.center = (p0 + p1) * 0.5f;
        sphere.radius = vector::length(p0 - p1) * 0.5f;
    }

    template <typename T>
    static sphere3<T> transform(const sphere3<T>& sphere, const mat4<T>& matrix) noexcept
    {
        vec4<T> center = {sphere.center.x, sphere.center.y, sphere.center.z, 1.0f};
        center = matrix::mul(center, matrix);

        return {
            .center = center,
            .radius = sphere.radius,
        };
    }
};
} // namespace violet