#pragma once

#include "type.hpp"
#include <cmath>

namespace violet
{
class vector
{
public:
    [[nodiscard]] static inline vector4f load(const float2& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(&v[0])));
#else
        return {v.x, v.y, 0.0f, 0.0f};
#endif
    }

    [[nodiscard]] static inline vector4f load(const float3& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 x = _mm_load_ss(&v[0]);
        __m128 y = _mm_load_ss(&v[1]);
        __m128 z = _mm_load_ss(&v[2]);
        __m128 xy = _mm_unpacklo_ps(x, y);
        return _mm_movelh_ps(xy, z);
#else
        return {v.x, v.y, v.z, 0.0f};
#endif
    }

    [[nodiscard]] static inline vector4f load(const float3& v, float w) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_load_ss(&v[0]);
        __m128 t2 = _mm_load_ss(&v[1]);
        __m128 t3 = _mm_load_ss(&v[2]);
        __m128 t4 = _mm_load_ss(&w);
        t1 = _mm_unpacklo_ps(t1, t2);
        t3 = _mm_unpacklo_ps(t3, t4);
        return _mm_movelh_ps(t1, t3);
#else
        return {v.x, v.y, v.z, w};
#endif
    }

    [[nodiscard]] static inline vector4f load(const float4& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_loadu_ps(&v[0]);
#else
        return {v.x, v.y, v.z, v.w};
#endif
    }

    [[nodiscard]] static inline vector4f load(const float4_align& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_load_ps(&v[0]);
#else
        return {v.x, v.y, v.z, v.w};
#endif
    }

    static inline void store(vector4f src, float2& dst) noexcept
    {
#ifdef VIOLET_USE_SIMD
        _mm_store_sd(reinterpret_cast<double*>(&dst[0]), _mm_castps_pd(src));
#else
        dst.x = src.x;
        dst.y = src.y;
#endif
    }

    static inline void store(vector4f src, float3& dst) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 y = simd::replicate<1>(src);
        __m128 z = simd::replicate<2>(src);

        _mm_store_ss(&dst.x, src);
        _mm_store_ss(&dst.y, y);
        _mm_store_ss(&dst.z, z);
#else
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
#endif
    }

    static inline void store(vector4f src, float4& dst) noexcept
    {
#ifdef VIOLET_USE_SIMD
        _mm_storeu_ps(&dst[0], src);
#else
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
        dst.w = src.w;
#endif
    }

    static inline void store(vector4f src, float4_align& dst) noexcept
    {
#ifdef VIOLET_USE_SIMD
        _mm_store_ps(&dst[0], src);
#else
        dst.x = src.x;
        dst.y = src.y;
        dst.z = src.z;
        dst.w = src.w;
#endif
    }

    template <typename T>
    [[nodiscard]] static inline T store(vector4f v) noexcept
    {
        T result;
        store(v, result);
        return result;
    }

    [[nodiscard]] static inline vector4f replicate(float v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_set_ps1(v);
#else
        return {v, v, v, v};
#endif
    }

    [[nodiscard]] static inline vector4f set(
        float x,
        float y = 0.0f,
        float z = 0.0f,
        float w = 0.0f) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_set_ps(w, z, y, x);
#else
        return {x, y, z, w};
#endif
    }

    [[nodiscard]] static inline float get_x(vector4f v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_cvtss_f32(v);
#else
        return v.x;
#endif
    }

    [[nodiscard]] static inline float get_y(vector4f v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_cvtss_f32(simd::shuffle<1, 1, 1, 1>(v));
#else
        return v.y;
#endif
    }

    [[nodiscard]] static inline float get_z(vector4f v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_cvtss_f32(simd::shuffle<2, 2, 2, 2>(v));
#else
        return v.z;
#endif
    }

    [[nodiscard]] static inline float get_w(vector4f v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_cvtss_f32(simd::shuffle<3, 3, 3, 3>(v));
#else
        return v.w;
#endif
    }

public:
    [[nodiscard]] static inline vector4f add(const vector4f& a, const vector4f& b) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_add_ps(a, b);
#else
        return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
#endif
    }

    [[nodiscard]] static inline vector4f sub(const vector4f& a, const vector4f& b) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_sub_ps(a, b);
#else
        return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
#endif
    }

    [[nodiscard]] static inline vector4f mul(const vector4f& a, const vector4f& b) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_mul_ps(a, b);
#else
        return {a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]};
#endif
    }

    [[nodiscard]] static inline vector4f mul(const vector4f& v, float scale) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 s = _mm_set_ps1(scale);
        return _mm_mul_ps(v, s);
#else
        return {v[0] * scale, v[1] * scale, v[2] * scale, v[3] * scale};
#endif
    }

    [[nodiscard]] static inline vector4f div(const vector4f& a, const vector4f& b) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_div_ps(a, b);
#else
        return {a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]};
#endif
    }

    [[nodiscard]] static inline float dot(const vector4f& a, const vector4f& b) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = dot_v(a, b);
        return _mm_cvtss_f32(t1);
#else
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
#endif
    }

    [[nodiscard]] static inline vector4f dot_v(vector4f a, vector4f b) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_mul_ps(a, b);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        return _mm_add_ps(t1, t2);
#else
        float dot_value = dot(a, b);
        return {dot_value, dot_value, dot_value, dot_value};
#endif
    }

    [[nodiscard]] static inline vector4f cross(const vector4f& a, const vector4f& b) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = simd::shuffle<1, 2, 0, 0>(a);
        __m128 t2 = simd::shuffle<2, 0, 1, 0>(b);
        __m128 t3 = _mm_mul_ps(t1, t2);

        t1 = simd::shuffle<2, 0, 1, 0>(a);
        t2 = simd::shuffle<1, 2, 0, 0>(b);
        t1 = _mm_mul_ps(t1, t2);
        t2 = _mm_sub_ps(t3, t1);

        return _mm_and_ps(t2, simd::mask_v<1, 1, 1, 0>);
#else
        return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
#endif
    }

    [[nodiscard]] static inline vector4f lerp(
        const vector4f& a,
        const vector4f& b,
        float m) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return lerp(a, b, _mm_set_ps1(m));
#else
        return {
            a[0] + m * (b[0] - a[0]),
            a[1] + m * (b[1] - a[1]),
            a[2] + m * (b[2] - a[2]),
            a[3] + m * (b[3] - a[3])};
#endif
    }

    [[nodiscard]] static inline vector4f lerp(
        const vector4f& a,
        const vector4f& b,
        const vector4f& m) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_sub_ps(b, a);
        t1 = _mm_mul_ps(t1, m);
        t1 = _mm_add_ps(a, t1);
        return t1;
#else
        return {
            a[0] + m[0] * (b[0] - a[0]),
            a[1] + m[1] * (b[1] - a[1]),
            a[2] + m[2] * (b[2] - a[2]),
            a[3] + m[3] * (b[3] - a[3])};
#endif
    }

    [[nodiscard]] static inline float length(const vector4f& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = length_v(v);
        return _mm_cvtss_f32(t1);
#else
        return sqrtf(dot(v, v));
#endif
    }

    [[nodiscard]] static inline vector4f length_v(vector4f v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = dot_v(v, v);
        return _mm_sqrt_ps(t1);
#else
        float length_value = length(v);
        return {length_value, length_value, length_value, length_value};
#endif
    }

    [[nodiscard]] static inline vector4f normalize(const vector4f& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_mul_ps(v, v);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        t1 = _mm_add_ps(t1, t2);

        t1 = _mm_sqrt_ps(t1);
        return _mm_div_ps(v, t1);
#else
        float s = 1.0f / length(v);
        return mul(v, s);
#endif
    }

    [[nodiscard]] static inline vector4f sqrt(const vector4f& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_sqrt_ps(v);
#else
        return {sqrtf(v[0]), sqrtf(v[1]), sqrtf(v[2]), sqrtf(v[3])};
#endif
    }

    [[nodiscard]] static inline vector4f reciprocal_sqrt(const vector4f& v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_rsqrt_ps(v);
#else
        return {1.0f / sqrtf(v[0]), 1.0f / sqrtf(v[1]), 1.0f / sqrtf(v[2]), 1.0f / sqrtf(v[3])};
#endif
    }
};
} // namespace violet