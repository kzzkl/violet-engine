#pragma once

#include "simd.hpp"
#include "type.hpp"

namespace ash::math
{
class vector
{
public:
    inline static float2 add(const float2& a, const float2& b)
    {
        return {a[0] + b[0], a[1] + b[1]};
    }

    inline static float3 add(const float3& a, const float3& b)
    {
        return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
    }

    inline static float4 add(const float4& a, const float4& b)
    {
        return {a[0] + b[0], a[1] + b[1], a[2] + b[2], a[3] + b[3]};
    }

    inline static float2 sub(const float2& a, const float2& b)
    {
        return {a[0] - b[0], a[1] - b[1]};
    }

    inline static float3 sub(const float3& a, const float3& b)
    {
        return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
    }

    inline static float4 sub(const float4& a, const float4& b)
    {
        return {a[0] - b[0], a[1] - b[1], a[2] - b[2], a[3] - b[3]};
    }

    inline static float2 mul(const float2& a, const float2& b)
    {
        return {a[0] * b[0], a[1] * b[1]};
    }

    inline static float3 mul(const float3& a, const float3& b)
    {
        return {a[0] * b[0], a[1] * b[1], a[2] * b[2]};
    }

    inline static float4 mul(const float4& a, const float4& b)
    {
        return {a[0] * b[0], a[1] * b[1], a[2] * b[2], a[3] * b[3]};
    }

    inline static float2 mul(const float2& v, float scale) { return {v[0] * scale, v[1] * scale}; }

    inline static float3 mul(const float3& v, float scale)
    {
        return {v[0] * scale, v[1] * scale, v[2] * scale};
    }

    inline static float4 mul(const float4& v, float scale)
    {
        return {v[0] * scale, v[1] * scale, v[2] * scale, v[3] * scale};
    }

    inline static float2 div(const float2& a, const float2& b)
    {
        return {a[0] / b[0], a[1] / b[1]};
    }

    inline static float3 div(const float3& a, const float3& b)
    {
        return {a[0] / b[0], a[1] / b[1], a[2] / b[2]};
    }

    inline static float4 div(const float4& a, const float4& b)
    {
        return {a[0] / b[0], a[1] / b[1], a[2] / b[2], a[3] / b[3]};
    }

    inline static float dot(const float2& a, const float2& b) { return a[0] * b[0] + a[1] * b[1]; }

    inline static float dot(const float3& a, const float3& b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    inline static float dot(const float4& a, const float4& b)
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
    }

    inline static float3 cross(const float3& a, const float3& b)
    {
        return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    }

    inline static float4 cross(const float4& a, const float4& b)
    {
        return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    }

    inline static float2 lerp(const float2& a, const float2& b, float m)
    {
        return {a[0] + m * (b[0] - a[0]), a[1] + m * (b[1] - a[1])};
    }

    inline static float3 lerp(const float3& a, const float3& b, float m)
    {
        return {a[0] + m * (b[0] - a[0]), a[1] + m * (b[1] - a[1]), a[2] + m * (b[2] - a[2])};
    }

    inline static float3 lerp(const float3& a, const float3& b, const float3& m)
    {
        return {
            a[0] + m[0] * (b[0] - a[0]),
            a[1] + m[1] * (b[1] - a[1]),
            a[2] + m[2] * (b[2] - a[2])};
    }

    inline static float4 lerp(const float4& a, const float4& b, float m)
    {
        return {
            a[0] + m * (b[0] - a[0]),
            a[1] + m * (b[1] - a[1]),
            a[2] + m * (b[2] - a[2]),
            a[3] + m * (b[3] - a[3])};
    }

    inline static float4 lerp(const float4& a, const float4& b, const float4& m)
    {
        return {
            a[0] + m[0] * (b[0] - a[0]),
            a[1] + m[1] * (b[1] - a[1]),
            a[2] + m[2] * (b[2] - a[2]),
            a[3] + m[3] * (b[3] - a[3])};
    }

    inline static float length(const float2& v) { return sqrtf(dot(v, v)); }

    inline static float length(const float3& v) { return sqrtf(dot(v, v)); }

    inline static float length(const float4& v) { return sqrtf(dot(v, v)); }

    inline static float2 normalize(const float2& v)
    {
        float s = 1.0f / length(v);
        return mul(v, s);
    }

    inline static float3 normalize(const float3& v)
    {
        float s = 1.0f / length(v);
        return mul(v, s);
    }

    inline static float4 normalize(const float4& v)
    {
        float s = 1.0f / length(v);
        return mul(v, s);
    }

    inline static float4 sqrt(const float4& v)
    {
        return {sqrtf(v[0]), sqrtf(v[1]), sqrtf(v[2]), sqrtf(v[3])};
    }

    inline static float4 reciprocal_sqrt(const float4& v)
    {
        return {1.0f / sqrtf(v[0]), 1.0f / sqrtf(v[1]), 1.0f / sqrtf(v[2]), 1.0f / sqrtf(v[3])};
    }
};

class vector_simd
{
public:
    inline static float4_simd add(float4_simd a, float4_simd b) { return _mm_add_ps(a, b); }
    inline static float4_simd sub(float4_simd a, float4_simd b) { return _mm_sub_ps(a, b); }
    inline static float4_simd mul(float4_simd a, float4_simd b) { return _mm_mul_ps(a, b); }
    inline static float4_simd mul(float4_simd v, float scale)
    {
        __m128 s = _mm_set_ps1(scale);
        return _mm_mul_ps(v, s);
    }
    inline static float4_simd div(float4_simd a, float4_simd b) { return _mm_div_ps(a, b); }

    inline static float dot(float4_simd a, float4_simd b)
    {
        __m128 t1 = dot_v(a, b);
        return _mm_cvtss_f32(t1);
    }

    inline static float4_simd dot_v(float4_simd a, float4_simd b)
    {
        __m128 t1 = _mm_mul_ps(a, b);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        return _mm_add_ps(t1, t2);
    }

    inline static float4_simd cross(float4_simd a, float4_simd b)
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

    inline static float4_simd lerp(float4_simd a, float4_simd b, float m)
    {
        return lerp(a, b, _mm_set_ps1(m));
    }

    inline static float4_simd lerp(float4_simd a, float4_simd b, float4_simd m)
    {
        __m128 t1 = _mm_sub_ps(b, a);
        t1 = _mm_mul_ps(t1, m);
        t1 = _mm_add_ps(a, t1);
        return t1;
    }

    inline static float length_vec3(float4_simd v)
    {
        __m128 t1 = length_vec3_v(_mm_and_ps(v, simd::mask_v<1, 1, 1, 0>));
        return _mm_cvtss_f32(t1);
    }

    inline static float4_simd length_vec3_v(float4_simd v)
    {
        return length_v(_mm_and_ps(v, simd::mask_v<1, 1, 1, 0>));
    }

    inline static float length(float4_simd v)
    {
        __m128 t1 = length_v(v);
        return _mm_cvtss_f32(t1);
    }

    inline static float4_simd length_v(float4_simd v)
    {
        __m128 t1 = dot_v(v, v);
        return _mm_sqrt_ps(t1);
    }

    inline static float4_simd normalize_vec3(float4_simd v)
    {
        return normalize(_mm_and_ps(v, simd::mask_v<1, 1, 1, 0>));
    }

    inline static float4_simd normalize(float4_simd v)
    {
        __m128 t1 = _mm_mul_ps(v, v);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        t1 = _mm_add_ps(t1, t2);

        t1 = _mm_sqrt_ps(t1);
        return _mm_div_ps(v, t1);
    }

    inline static float4_simd sqrt(float4_simd v) { return _mm_sqrt_ps(v); }

    inline static float4_simd reciprocal_sqrt(float4_simd v)
    {
        __m128 sqrt = _mm_sqrt_ps(v);
        __m128 one = simd::set(1.0f);

        return _mm_div_ps(one, sqrt);
    }

    inline static float4_simd reciprocal_sqrt_fast(float4_simd v) { return _mm_rsqrt_ps(v); }
};
} // namespace ash::math