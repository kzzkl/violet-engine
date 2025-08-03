#pragma once

#include "math/matrix.hpp"
#include "math/vector.hpp"
#include <ranges>

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

template <typename T>
concept is_sphere3 = requires(T v) {
    { sphere3{v} } -> std::same_as<T>;
};

struct sphere
{
    template <std::ranges::contiguous_range R>
    static auto create(R&& points) noexcept
        requires is_vec3<std::ranges::range_value_t<R>>
    {
        using value_type = std::ranges::range_value_t<R>::value_type;
        using sphere_type = sphere3<value_type>;

        if (points.empty())
        {
            return sphere_type{};
        }

        std::uint32_t min_index[3] = {0, 0, 0};
        std::uint32_t max_index[3] = {0, 0, 0};

        for (std::uint32_t i = 0; i < points.size(); ++i)
        {
            for (std::uint32_t j = 0; j < 3; ++j)
            {
                if (points[i][j] < points[min_index[j]][j])
                {
                    min_index[j] = i;
                }

                if (points[i][j] > points[max_index[j]][j])
                {
                    max_index[j] = i;
                }
            }
        }

        value_type largest_distance_sq = value_type(0);
        std::uint32_t largest_axis = 0;

        for (std::uint32_t i = 0; i < 3; ++i)
        {
            const auto& min_point = points[min_index[i]];
            const auto& max_point = points[max_index[i]];

            auto distance_sq = vector::length_sq(min_point - max_point);

            if (distance_sq > largest_distance_sq)
            {
                largest_distance_sq = distance_sq;
                largest_axis = i;
            }
        }

        sphere_type result;
        result.center = (points[min_index[largest_axis]] + points[max_index[largest_axis]]) * 0.5f;

        value_type largest_radius_sq = value_type(0);
        for (const auto& point : points)
        {
            auto distance_sq = vector::length_sq(point - result.center);
            largest_radius_sq = std::max(largest_radius_sq, distance_sq);
        }
        result.radius = std::sqrt(largest_radius_sq);

        return result;
    }

    template <std::ranges::contiguous_range R>
    static auto create(R&& spheres) noexcept
        requires is_sphere3<std::ranges::range_value_t<R>>
    {
        using sphere_type = std::ranges::range_value_t<R>;
        using value_type = sphere_type::value_type;

        std::uint32_t min_index[3] = {0, 0, 0};
        std::uint32_t max_index[3] = {0, 0, 0};

        for (std::uint32_t i = 0; i < spheres.size(); ++i)
        {
            for (std::uint32_t j = 0; j < 3; ++j)
            {
                if (spheres[i].center[j] - spheres[i].radius <
                    spheres[min_index[j]].center[j] - spheres[min_index[j]].radius)
                {
                    min_index[j] = i;
                }

                if (spheres[i].center[j] + spheres[i].radius >
                    spheres[max_index[j]].center[j] + spheres[max_index[j]].radius)
                {
                    max_index[j] = i;
                }
            }
        }

        value_type largest_distance = value_type(0);
        std::uint32_t largest_axis = 0;

        for (std::uint32_t i = 0; i < 3; ++i)
        {
            const auto& min_sphere = spheres[min_index[i]];
            const auto& max_sphere = spheres[max_index[i]];

            auto distance = vector::length(min_sphere.center - max_sphere.center) +
                            min_sphere.radius + max_sphere.radius;

            if (distance > largest_distance)
            {
                largest_distance = distance;
                largest_axis = i;
            }
        }

        sphere_type result = spheres[min_index[largest_axis]];
        expand(result, spheres[max_index[largest_axis]]);

        for (const auto& sphere : spheres)
        {
            expand(result, sphere);
        }

        return result;
    }

    template <typename T>
    static void expand(sphere3<T>& sphere, const vec3<T>& point) noexcept
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
    static void expand(sphere3<T>& sphere, const sphere3<T>& other) noexcept
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

        float distance = vector::length(other.center - sphere.center);
        if (distance + sphere.radius <= other.radius)
        {
            sphere = other;
            return;
        }

        if (distance + other.radius <= sphere.radius)
        {
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