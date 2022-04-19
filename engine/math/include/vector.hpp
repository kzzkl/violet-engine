#pragma once

#include "simd.hpp"
#include "type.hpp"

namespace ash::math
{
class vector_plain
{
public:
    using vector2 = float2;
    using vector3 = float3;
    using vector4 = float4;
    using vector_type = float4;

public:
    inline static float2 add(const vector2& a, const vector2& b)
    {
        return {a[0] + b[0], a[1] + b[1]};
    }

    inline static float3 add(const vector3& a, const vector3& b)
    {
        return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
    }

    inline static vector_type add(const vector_type& a, const vector_type& b)
    {
        return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
    }

    inline static float2 sub(const vector2& a, const vector2& b)
    {
        return {a[0] - b[0], a[1] - b[1]};
    }

    inline static float3 sub(const vector3& a, const vector3& b)
    {
        return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    }

    inline static vector_type sub(const vector_type& a, const vector_type& b)
    {
        return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
    }

    inline static float dot(const vector2& a, const vector2& b)
    {
        return a[0] * b[0] + a[1] * b[1];
    }

    inline static float dot(const vector3& a, const vector3& b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    inline static float dot(const vector_type& a, const vector_type& b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    }

    inline static vector3 cross(const vector3& a, const vector3& b)
    {
        return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    }

    inline static vector_type cross(const vector_type& a, const vector_type& b)
    {
        return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    }

    inline static vector2 scale(const vector2& v, float scale)
    {
        return {v[0] * scale, v[1] * scale};
    }

    inline static vector3 scale(const vector3& v, float scale)
    {
        return {v[0] * scale, v[1] * scale, v[2] * scale};
    }

    inline static vector_type scale(const vector_type& v, float scale)
    {
        return {v[0] * scale, v[1] * scale, v[2] * scale, v[3] * scale};
    }

    inline static vector2 lerp(const vector2& a, const vector2& b, float m)
    {
        return {a[0] + m * (b[0] - a[0]), a[1] + m * (b[1] - a[1])};
    }

    inline static vector3 lerp(const vector3& a, const vector3& b, float m)
    {
        return {a[0] + m * (b[0] - a[0]), a[1] + m * (b[1] - a[1]), a[2] + m * (b[2] - a[2])};
    }

    inline static vector3 lerp(const vector3& a, const vector3& b, const vector3& m)
    {
        return {
            a[0] + m[0] * (b[0] - a[0]),
            a[1] + m[1] * (b[1] - a[1]),
            a[2] + m[2] * (b[2] - a[2])};
    }

    inline static vector_type lerp(const vector_type& a, const vector_type& b, float m)
    {
        return {
            a[0] + m * (b[0] - a[0]),
            a[1] + m * (b[1] - a[1]),
            a[2] + m * (b[2] - a[2]),
            a[3] + m * (b[3] - a[3])};
    }

    inline static float length(const vector2& v) { return sqrtf(dot(v, v)); }

    inline static float length(const vector3& v) { return sqrtf(dot(v, v)); }

    inline static float length(const vector_type& v) { return sqrtf(dot(v, v)); }

    inline static vector2 normalize(const vector2& v)
    {
        float s = 1.0f / length(v);
        return scale(v, s);
    }

    inline static vector3 normalize(const vector3& v)
    {
        float s = 1.0f / length(v);
        return scale(v, s);
    }

    inline static vector_type normalize(const vector_type& v)
    {
        float s = 1.0f / length(v);
        return scale(v, s);
    }

    inline static vector_type sqrt(const vector_type& v)
    {
        return {sqrtf(v[0]), sqrtf(v[1]), sqrtf(v[2]), sqrtf(v[3])};
    }

    inline static vector_type reciprocal_sqrt(const vector_type& v)
    {
        return {1.0f / sqrtf(v[0]), 1.0f / sqrtf(v[1]), 1.0f / sqrtf(v[2]), 1.0f / sqrtf(v[3])};
    }
};

class vector_simd
{
public:
    using vector_type = float4_simd;

public:
    inline static vector_type add(const vector_type& a, const vector_type& b)
    {
        return _mm_add_ps(a, b);
    }

    inline static vector_type sub(const vector_type& a, const vector_type& b)
    {
        return _mm_sub_ps(a, b);
    }

    inline static float dot(const vector_type& a, const vector_type& b)
    {
        __m128 t1 = dot_v(a, b);
        return _mm_cvtss_f32(t1);
    }

    inline static vector_type dot_v(const vector_type& a, const vector_type& b)
    {
        __m128 t1 = _mm_mul_ps(a, b);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        return _mm_add_ps(t1, t2);
    }

    inline static vector_type cross(const vector_type& a, const vector_type& b)
    {
        __m128 t1 = simd::shuffle<1, 2, 0, 0>(a);
        __m128 t2 = simd::shuffle<2, 0, 1, 0>(b);
        __m128 t3 = _mm_mul_ps(t1, t2);

        t1 = simd::shuffle<2, 0, 1, 0>(a);
        t2 = simd::shuffle<1, 2, 0, 0>(b);
        t1 = _mm_mul_ps(t1, t2);
        t2 = _mm_sub_ps(t3, t1);

        return _mm_and_ps(t2, simd::mask<0x1110>());
    }

    inline static vector_type scale(const vector_type& v, float scale)
    {
        __m128 s = _mm_set_ps1(scale);
        return _mm_mul_ps(v, s);
    }

    inline static float length(const vector_type& v)
    {
        __m128 t1 = length_v(v);
        return _mm_cvtss_f32(t1);
    }

    inline static vector_type length_v(const vector_type& v)
    {
        __m128 t1 = dot_v(v, v);
        return _mm_sqrt_ps(t1);
    }

    inline static vector_type normalize_vec3(const vector_type& v)
    {
        return normalize(_mm_and_ps(v, simd::mask<0x1110>()));
    }

    inline static vector_type normalize(const vector_type& v)
    {
        __m128 t1 = _mm_mul_ps(v, v);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        t1 = _mm_add_ps(t1, t2);

        t1 = _mm_sqrt_ps(t1);
        return _mm_div_ps(v, t1);
    }

    inline static vector_type sqrt(const vector_type& v) { return _mm_sqrt_ps(v); }

    inline static vector_type reciprocal_sqrt(const vector_type& v)
    {
        __m128 sqrt = _mm_sqrt_ps(v);
        __m128 one = simd::set(1.0f);

        return _mm_div_ps(one, sqrt);
    }

    inline static vector_type reciprocal_sqrt_fast(const vector_type& v) { return _mm_rsqrt_ps(v); }
};
} // namespace ash::math