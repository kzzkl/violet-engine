#pragma once

#include "math/box.hpp"

namespace violet
{
template <typename T>
struct ray3
{
    struct hit_result
    {
        T enter{0};
        T exit{-1};

        operator bool() const noexcept
        {
            return exit >= enter;
        }
    };

    static constexpr hit_result no_hit = {};

    vec3<T> origin;
    vec3<T> direction;

    hit_result intersect(const box3<T>& box) const noexcept
    {
        hit_result result;

        T tx1 = (box.min.x - origin.x) / direction.x;
        T tx2 = (box.max.x - origin.x) / direction.x;

        result.enter = std::min(tx1, tx2);
        result.exit = std::max(tx1, tx2);

        T ty1 = (box.min.y - origin.y) / direction.y;
        T ty2 = (box.max.y - origin.y) / direction.y;

        result.enter = std::max(result.enter, std::min(ty1, ty2));
        result.exit = std::min(result.exit, std::max(ty1, ty2));

        T tz1 = (box.min.z - origin.z) / direction.z;
        T tz2 = (box.max.z - origin.z) / direction.z;

        result.enter = std::max(result.enter, std::min(tz1, tz2));
        result.exit = std::min(result.exit, std::max(tz1, tz2));

        return result;
    }

    hit_result intersect(const vec3<T>& v0, const vec3<T>& v1, const vec3<T>& v2) const noexcept
    {
        constexpr T epsilon = std::numeric_limits<T>::epsilon() * T(100);

        vec3<T> edge1 = v1 - v0;
        vec3<T> edge2 = v2 - v0;
        vec3<T> ray_cross_e2 = vector::cross(direction, edge2);
        T det = vector::dot(edge1, ray_cross_e2);

        if (det > -epsilon && det < epsilon)
        {
            return no_hit;
        }

        T inv_det = T(1) / det;
        vec3<T> s = origin - v0;
        T u = inv_det * vector::dot(s, ray_cross_e2);

        if (u < 0 || u > 1)
        {
            return no_hit;
        }

        vec3<T> s_cross_e1 = vector::cross(s, edge1);
        T v = inv_det * vector::dot(direction, s_cross_e1);

        if (v < 0 || u + v > 1)
        {
            return no_hit;
        }

        T t = inv_det * vector::dot(edge2, s_cross_e1);

        if (t < epsilon)
        {
            return no_hit;
        }

        return {
            .enter = t,
            .exit = t,
        };
    }
};

using ray3f = ray3<float>;
} // namespace violet