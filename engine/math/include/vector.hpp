#pragma once

#include "simd.hpp"
#include "type.hpp"

namespace ash::math
{
struct vector
{
    template <row_vector T1, row_vector T2>
    inline static T1 add(const T1& a, const T2& b) requires col_size_equal<T1, T2>
    {
        T1 result = a;
        for (std::size_t i = 0; i < packed_trait<T1>::col_size; ++i)
            result[i] += b[i];
        return result;
    }

    template <>
    inline static float4_simd add(const float4_simd& a, const float4_simd& b)
    {
        return _mm_add_ps(a, b);
    }

    template <row_vector T1, row_vector T2>
    inline static T1 sub(const T1& a, const T2& b) requires col_size_equal<T1, T2>
    {
        T1 result = a;
        for (std::size_t i = 0; i < packed_trait<T1>::col_size; ++i)
            result[i] -= b[i];
        return result;
    }

    template <>
    inline static float4_simd sub(const float4_simd& a, const float4_simd& b)
    {
        return _mm_sub_ps(a, b);
    }

    template <row_vector T1, row_vector T2>
    inline static packed_trait<T1>::value_type dot(
        const T1& a,
        const T2& b) requires col_size_equal<T1, T2>
    {
        typename packed_trait<T1>::value_type result = 0;
        for (std::size_t i = 0; i < packed_trait<T1>::col_size; ++i)
            result += a[i] * b[i];
        return result;
    }

    template <>
    inline static float4_simd dot(const float4_simd& a, const float4_simd& b)
    {
        __m128 t1 = _mm_mul_ps(a, b);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        return _mm_add_ps(t1, t2);
    }

    template <vector_1x3_1x4 T1, vector_1x3_1x4 T2>
    inline static T1 cross(const T1& a, const T2& b) requires col_size_equal<T1, T2>
    {
        return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
    }

    template <>
    inline static float4_simd cross(const float4_simd& a, const float4_simd& b)
    {
        __m128 t1 = simd::shuffle<1, 2, 0, 0>(a);
        __m128 t2 = simd::shuffle<2, 0, 1, 0>(b);
        __m128 t3 = _mm_mul_ps(t1, t2);

        t1 = simd::shuffle<2, 0, 1, 0>(a);
        t2 = simd::shuffle<1, 2, 0, 0>(b);
        t1 = _mm_mul_ps(t1, t2);

        return _mm_and_ps(_mm_sub_ps(t3, t1), simd::get_mask<0x1110>());
    }

    template <row_vector T, typename S>
    inline static T scale(const T& v, S scale)
    {
        T result = v;
        for (std::size_t i = 0; i < packed_trait<T>::col_size; ++i)
            result[i] *= scale;
        return result;
    }

    template <>
    inline static float4_simd scale(const float4_simd& v, float scale)
    {
        __m128 s = _mm_set_ps1(scale);
        return _mm_mul_ps(v, s);
    }

    template <row_vector T>
    inline static packed_trait<T>::value_type length(const T& v)
    {
        return sqrtf(dot(v, v));
    }

    template <>
    inline static float4_simd length(const float4_simd& v)
    {
        __m128 t1 = dot(v, v);
        return _mm_sqrt_ps(t1);
    }

    template <row_vector T>
    inline static T normalize(const T& v)
    {
        float s = 1.0f / length(v);
        return scale(v, s);
    }

    template <>
    inline static float4_simd normalize(const float4_simd& v)
    {
        __m128 t1 = _mm_mul_ps(v, v);
        __m128 t2 = simd::shuffle<1, 0, 3, 2>(t1);
        t1 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<2, 3, 0, 1>(t1);
        t1 = _mm_add_ps(t1, t2);

        t1 = _mm_sqrt_ps(t1);
        return _mm_div_ps(v, t1);
    }
};
} // namespace ash::math