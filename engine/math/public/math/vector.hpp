#pragma once

#include "math/math.hpp"
#include "math/types.hpp"

namespace violet
{
struct vector
{
    template <typename T>
    [[nodiscard]] static inline vec2<T> add(const vec2<T>& a, const vec2<T>& b) noexcept
    {
        return {a.x + b.x, a.y + b.y};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> add(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> add(const vec4<T>& a, const vec4<T>& b) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_add_ps(a, b);
        }
        else
        {
            return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> sub(const vec2<T>& a, const vec2<T>& b) noexcept
    {
        return {a.x - b.x, a.y - b.y};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> sub(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> sub(const vec4<T>& a, const vec4<T>& b) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_sub_ps(a, b);
        }
        else
        {
            return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> mul(const vec2<T>& a, const vec2<T>& b) noexcept
    {
        return {a.x * b.x, a.y * b.y};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> mul(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return {a.x * b.x, a.y * b.y, a.z * b.z};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> mul(const vec4<T>& a, const vec4<T>& b) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_mul_ps(a, b);
        }
        else
        {
            return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> mul(const vec2<T>& v, vec2<T>::value_type scale) noexcept
    {
        return {v.x * scale, v.y * scale};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> mul(const vec3<T>& v, vec3<T>::value_type scale) noexcept
    {
        return {v.x * scale, v.y * scale, v.z * scale};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> mul(const vec4<T>& v, vec4<T>::value_type scale) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 s = _mm_set_ps1(scale);
            return _mm_mul_ps(v, s);
        }
        else
        {
            return {v.x * scale, v.y * scale, v.z * scale, v.w * scale};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> div(const vec2<T>& a, const vec2<T>& b) noexcept
    {
        return {a.x / b.x, a.y / b.y};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> div(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return {a.x / b.x, a.y / b.y, a.z / b.z};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> div(const vec4<T>& a, const vec4<T>& b) noexcept
    {
        return {a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w};
    }

    [[nodiscard]] static inline vec4f_simd div(vec4f_simd a, vec4f_simd b) noexcept
    {
        return _mm_div_ps(a, b);
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> div(const vec2<T>& v, vec2<T>::value_type scale) noexcept
    {
        return {v.x / scale, v.y / scale};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> div(const vec3<T>& v, vec3<T>::value_type scale) noexcept
    {
        return {v.x / scale, v.y / scale, v.z / scale};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> div(const vec4<T>& v, vec4<T>::value_type scale) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 s = _mm_set_ps1(scale);
            return _mm_div_ps(v, s);
        }
        else
        {
            return {v.x / scale, v.y / scale, v.z / scale, v.w / scale};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T>::value_type dot(const vec2<T>& a, const vec2<T>& b) noexcept
    {
        return a.x * b.x + a.y * b.y;
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T>::value_type dot(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T>::value_type dot(const vec4<T>& a, const vec4<T>& b) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 t1 = dot_v(a, b);
            return _mm_cvtss_f32(t1);
        }
        else
        {
            return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
        }
    }

    [[nodiscard]] static inline vec4f_simd dot_v(vec4f_simd a, vec4f_simd b) noexcept
    {
        __m128 t1 = _mm_mul_ps(a, b);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        return _mm_add_ps(t1, t2);
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> cross(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    [[nodiscard]] static inline vec4f_simd cross(vec4f_simd a, vec4f_simd b) noexcept
    {
        __m128 t1 = simd::shuffle<1, 2, 0, 0>(a);
        __m128 t2 = simd::shuffle<2, 0, 1, 0>(b);
        __m128 t3 = _mm_mul_ps(t1, t2);

        t1 = simd::shuffle<2, 0, 1, 0>(a);
        t2 = simd::shuffle<1, 2, 0, 0>(b);
        t1 = _mm_mul_ps(t1, t2);
        t2 = _mm_sub_ps(t3, t1);

        return _mm_and_ps(t2, simd::mask_v<1, 1, 1, 0>);
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> lerp(const vec2<T>& a, const vec2<T>& b, float t) noexcept
        requires std::is_floating_point_v<T>
    {
        return {math::lerp(a.x, b.x, t), math::lerp(a.y, b.y, t)};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> lerp(const vec3<T>& a, const vec3<T>& b, float t) noexcept
        requires std::is_floating_point_v<T>
    {
        return {math::lerp(a.x, b.x, t), math::lerp(a.y, b.y, t), math::lerp(a.z, b.z, t)};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> lerp(const vec4<T>& a, const vec4<T>& b, float t) noexcept
        requires std::is_floating_point_v<T>
    {
        return {
            math::lerp(a.x, b.x, t),
            math::lerp(a.y, b.y, t),
            math::lerp(a.z, b.z, t),
            math::lerp(a.w, b.w, t)};
    }

    [[nodiscard]] static inline vec4f_simd lerp(vec4f_simd a, vec4f_simd b, float t) noexcept
    {
        return lerp(a, b, _mm_set_ps1(t));
    }

    [[nodiscard]] static inline vec4f_simd lerp(vec4f_simd a, vec4f_simd b, vec4f_simd t) noexcept
    {
        __m128 t1 = _mm_sub_ps(b, a);
        t1 = _mm_mul_ps(t1, t);
        t1 = _mm_add_ps(a, t1);
        return t1;
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T>::value_type length(const vec2<T>& v) noexcept
        requires std::is_floating_point_v<T>
    {
        return std::sqrt(length_sq(v));
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T>::value_type length(const vec3<T>& v) noexcept
        requires std::is_floating_point_v<T>
    {
        return std::sqrt(length_sq(v));
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T>::value_type length(const vec4<T>& v) noexcept
        requires std::is_floating_point_v<T>
    {
        return std::sqrt(length_sq(v));
    }

    [[nodiscard]] static inline float length(vec4f_simd v) noexcept
    {
        __m128 t1 = length_v(v);
        return _mm_cvtss_f32(t1);
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T>::value_type length_sq(const vec2<T>& v) noexcept
    {
        return dot(v, v);
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T>::value_type length_sq(const vec3<T>& v) noexcept
    {
        return dot(v, v);
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T>::value_type length_sq(const vec4<T>& v) noexcept
    {
        return dot(v, v);
    }

    [[nodiscard]] static inline vec4f_simd length_v(vec4f_simd v) noexcept
    {
        __m128 t1 = dot_v(v, v);
        return _mm_sqrt_ps(t1);
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> normalize(const vec2<T>& v) noexcept
        requires std::is_floating_point_v<T>
    {
        return div(v, length());
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> normalize(const vec3<T>& v) noexcept
        requires std::is_floating_point_v<T>
    {
        return div(v, length(v));
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> normalize(const vec4<T>& v) noexcept
        requires std::is_floating_point_v<T>
    {
        return div(v, length(v));
    }

    [[nodiscard]] static inline vec4f_simd normalize(vec4f_simd v) noexcept
    {
        v = _mm_and_ps(v, simd::mask_v<1, 1, 1, 0>);
        __m128 t1 = _mm_mul_ps(v, v);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        t1 = _mm_add_ps(t1, t2);

        t1 = _mm_sqrt_ps(t1);
        return _mm_div_ps(v, t1);
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> sqrt(const vec4<T>& v) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_sqrt_ps(v);
        }
        else
        {
            return {std::sqrt(v.x), std::sqrt(v.y), std::sqrt(v.z), std::sqrt(v.w)};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> reciprocal_sqrt(const vec4<T>& v) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_rsqrt_ps(v);
        }
        else
        {
            using value_type = vec4<T>::value_type;
            return {
                value_type(1) / std::sqrt(v.x),
                value_type(1) / std::sqrt(v.y),
                value_type(1) / std::sqrt(v.z),
                value_type(1) / std::sqrt(v.w)};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> min(const vec2<T>& a, const vec2<T>& b) noexcept
    {
        return {std::min(a.x, b.x), std::min(a.y, b.y)};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> min(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> min(const vec4<T>& a, const vec4<T>& b) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_min_ps(a, b);
        }
        else
        {
            return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z), std::min(a.w, b.w)};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec2<T> max(const vec2<T>& a, const vec2<T>& b) noexcept
    {
        return {std::max(a.x, b.x), std::max(a.y, b.y)};
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> max(const vec3<T>& a, const vec3<T>& b) noexcept
    {
        return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> max(const vec4<T>& a, const vec4<T>& b) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_max_ps(a, b);
        }
        else
        {
            return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z), std::max(a.w, b.w)};
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec3<T> clamp(
        const vec3<T>& value,
        const vec3<T>& min,
        const vec3<T>& max)
    {
        vec3<T> result;
        result = vector::min(value, max);
        result = vector::max(result, min);
        return result;
    }

    [[nodiscard]] static inline vec4f_simd replicate(float v) noexcept
    {
        return _mm_set_ps1(v);
    }

    [[nodiscard]] static inline vec4f_simd set(
        float x,
        float y = 0.0f,
        float z = 0.0f,
        float w = 0.0f) noexcept
    {
        return _mm_set_ps(w, z, y, x);
    }

    [[nodiscard]] static inline float get_x(vec4f_simd v) noexcept
    {
        return _mm_cvtss_f32(v);
    }

    [[nodiscard]] static inline float get_y(vec4f_simd v) noexcept
    {
        return _mm_cvtss_f32(simd::shuffle<1, 1, 1, 1>(v));
    }

    [[nodiscard]] static inline float get_z(vec4f_simd v) noexcept
    {
        return _mm_cvtss_f32(simd::shuffle<2, 2, 2, 2>(v));
    }

    [[nodiscard]] static inline float get_w(vec4f_simd v) noexcept
    {
        return _mm_cvtss_f32(simd::shuffle<3, 3, 3, 3>(v));
    }
};

template <typename T>
inline vec2<T> operator+(const vec2<T>& a, const vec2<T>& b)
{
    return vector::add(a, b);
}

template <typename T>
inline vec3<T> operator+(const vec3<T>& a, const vec3<T>& b)
{
    return vector::add(a, b);
}

template <typename T>
inline vec4<T> operator+(const vec4<T>& a, const vec4<T>& b)
{
    return vector::add(a, b);
}

template <typename T>
inline vec2<T> operator-(const vec2<T>& a, const vec2<T>& b)
{
    return vector::sub(a, b);
}

template <typename T>
inline vec3<T> operator-(const vec3<T>& a, const vec3<T>& b)
{
    return vector::sub(a, b);
}

template <typename T>
inline vec4<T> operator-(const vec4<T>& a, const vec4<T>& b)
{
    return vector::sub(a, b);
}

template <typename T>
inline vec2<T> operator*(const vec2<T>& a, const vec2<T>& b)
{
    return vector::mul(a, b);
}

template <typename T>
inline vec3<T> operator*(const vec3<T>& a, const vec3<T>& b)
{
    return vector::mul(a, b);
}

template <typename T>
inline vec4<T> operator*(const vec4<T>& a, const vec4<T>& b)
{
    return vector::mul(a, b);
}

template <typename T>
inline vec2<T> operator*(const vec2<T>& a, typename vec2<T>::value_type scale)
{
    return vector::mul(a, scale);
}

template <typename T>
inline vec3<T> operator*(const vec3<T>& a, typename vec3<T>::value_type scale)
{
    return vector::mul(a, scale);
}

template <typename T>
inline vec4<T> operator*(const vec4<T>& a, typename vec4<T>::value_type scale)
{
    return vector::mul(a, scale);
}

template <typename T>
inline vec2<T> operator/(const vec2<T>& a, const vec2<T>& b)
{
    return vector::div(a, b);
}

template <typename T>
inline vec3<T> operator/(const vec3<T>& a, const vec3<T>& b)
{
    return vector::div(a, b);
}

template <typename T>
inline vec4<T> operator/(const vec4<T>& a, const vec4<T>& b)
{
    return vector::div(a, b);
}

template <typename T>
inline vec2<T> operator/(const vec2<T>& a, typename vec2<T>::value_type scale)
{
    return vector::div(a, scale);
}

template <typename T>
inline vec3<T> operator/(const vec3<T>& a, typename vec3<T>::value_type scale)
{
    return vector::div(a, scale);
}

template <typename T>
inline vec4<T> operator/(const vec4<T>& a, typename vec4<T>::value_type scale)
{
    return vector::div(a, scale);
}
} // namespace violet