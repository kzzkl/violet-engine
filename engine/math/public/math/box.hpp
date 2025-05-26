#pragma once

#include "math/matrix.hpp"
#include "math/vector.hpp"
#include <limits>

namespace violet
{
template <typename T>
struct box3
{
    using value_type = T;

    box3()
        : min{std::numeric_limits<value_type>::max(),
              std::numeric_limits<value_type>::max(),
              std::numeric_limits<value_type>::max()},
          max{std::numeric_limits<value_type>::lowest(),
              std::numeric_limits<value_type>::lowest(),
              std::numeric_limits<value_type>::lowest()}
    {
    }

    vec3<T> min;
    vec3<T> max;

    [[nodiscard]] bool operator==(const box3& other) const noexcept
    {
        return min == other.min && max == other.max;
    }

    [[nodiscard]] bool operator!=(const box3& other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] operator bool() const noexcept
    {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }
};

template <>
struct box3<simd>
{
    using value_type = float;
    using vec3_type = vec3<simd>;

    box3()
        : min(vector::replicate(std::numeric_limits<value_type>::max())),
          max(vector::replicate(std::numeric_limits<value_type>::lowest()))
    {
    }

    vec4f_simd min;
    vec4f_simd max;
};

using box3f = box3<float>;
using box3f_simd = box3<simd>;

struct box
{
    template <typename T>
    static inline void expand(box3<T>& box, const vec3<T>& point) noexcept
    {
        box.min = vector::min(box.min, point);
        box.max = vector::max(box.max, point);
    }

    static inline void expand(box3f_simd& box, const vec4f_simd& point) noexcept
    {
        box.min = vector::min(box.min, point);
        box.max = vector::max(box.max, point);
    }

    template <typename T>
    static inline void expand(box3<T>& box, const box3<T>& other) noexcept
    {
        box.min = vector::min(box.min, other.min);
        box.max = vector::max(box.max, other.max);
    }

    static inline void expand(box3f_simd& box, const box3f_simd& other) noexcept
    {
        box.min = vector::min(box.min, other.min);
        box.max = vector::max(box.max, other.max);
    }

    template <typename T>
    static vec3<T> get_center(const box3<T>& box) noexcept
    {
        using value_type = vec3<T>::value_type;
        return (box.min + box.max) * value_type(0.5);
    }

    template <typename T>
    static vec3<T> get_extent(const box3<T>& box) noexcept
    {
        return box.max - box.min;
    }

    template <typename T>
    static box3<T> transform(const box3<T>& box, const mat4<T>& matrix) noexcept
    {
        vec4<T> corners[8] = {
            matrix::mul({box.min.x, box.min.y, box.min.z, 1.0}, matrix),
            matrix::mul({box.min.x, box.max.y, box.min.z, 1.0}, matrix),
            matrix::mul({box.min.x, box.min.y, box.max.z, 1.0}, matrix),
            matrix::mul({box.min.x, box.max.y, box.max.z, 1.0}, matrix),
            matrix::mul({box.max.x, box.min.y, box.min.z, 1.0}, matrix),
            matrix::mul({box.max.x, box.max.y, box.min.z, 1.0}, matrix),
            matrix::mul({box.max.x, box.min.y, box.max.z, 1.0}, matrix),
            matrix::mul({box.max.x, box.max.y, box.max.z, 1.0}, matrix),
        };

        box3<T> result;
        for (auto& corner : corners)
        {
            expand(result, (vec3<T>)corner);
        }

        return result;
    }
};
} // namespace violet