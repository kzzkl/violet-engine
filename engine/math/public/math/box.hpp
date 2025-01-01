#pragma once

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
          max{std::numeric_limits<value_type>::min(),
              std::numeric_limits<value_type>::min(),
              std::numeric_limits<value_type>::min()}
    {
    }

    vec3<T> min;
    vec3<T> max;
};

template <>
struct box3<simd>
{
    using value_type = float;
    using vec3_type = vec3<simd>;

    box3()
        : min(vector::replicate(std::numeric_limits<value_type>::max())),
          max(vector::replicate(std::numeric_limits<value_type>::min()))
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
    static vec3<T> get_center(const box3<T>& box) noexcept
    {
        using value_type = vec3<T>::value_type;
        return (box.min + box.max) * value_type(0.5);
    }
};
} // namespace violet