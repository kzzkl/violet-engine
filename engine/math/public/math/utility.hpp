#pragma once

#include "math/types.hpp"

namespace violet
{
class math
{
public:
    static constexpr float PI = 3.141592654f;
    static constexpr float TWO_PI = 2.0f * PI;
    static constexpr float INV_PI = 1.0f / PI;
    static constexpr float INV_TWO_PI = 1.0f / TWO_PI;
    static constexpr float HALF_PI = PI / 2.0f;
    static constexpr float QUARTER_PI = PI / 4.0f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;
    static constexpr float RAD_TO_DEG = 180.0f / PI;

#ifdef VIOLET_USE_SIMD
    static constexpr simd::convert<float> IDENTITY_ROW_0 = {1.0f, 0.0f, 0.0f, 0.0f};
    static constexpr simd::convert<float> IDENTITY_ROW_1 = {0.0f, 1.0f, 0.0f, 0.0f};
    static constexpr simd::convert<float> IDENTITY_ROW_2 = {0.0f, 0.0f, 1.0f, 0.0f};
    static constexpr simd::convert<float> IDENTITY_ROW_3 = {0.0f, 0.0f, 0.0f, 1.0f};
#else
    static constexpr matrix4::row_type IDENTITY_ROW_0 = {1.0f, 0.0f, 0.0f, 0.0f};
    static constexpr matrix4::row_type IDENTITY_ROW_1 = {0.0f, 1.0f, 0.0f, 0.0f};
    static constexpr matrix4::row_type IDENTITY_ROW_2 = {0.0f, 0.0f, 1.0f, 0.0f};
    static constexpr matrix4::row_type IDENTITY_ROW_3 = {0.0f, 0.0f, 0.0f, 1.0f};
#endif

public:
    [[nodiscard]] static inline vector4f load(float v) noexcept
    {
#ifdef VIOLET_USE_SIMD
        return _mm_set_ps1(v);
#else
        return {v, v, v, v};
#endif
    }

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

    [[nodiscard]] static inline matrix4 load(const float4x4& m)
    {
#ifdef VIOLET_USE_SIMD
        return {
            _mm_loadu_ps(&m[0][0]),
            _mm_loadu_ps(&m[1][0]),
            _mm_loadu_ps(&m[2][0]),
            _mm_loadu_ps(&m[3][0])};
#else
        return {m[0], m[1], m[2], m[3]};
#endif
    }

    [[nodiscard]] static inline matrix4 load(const float4x4_align& m)
    {
#ifdef VIOLET_USE_SIMD
        return {
            _mm_load_ps(&m[0][0]),
            _mm_load_ps(&m[1][0]),
            _mm_load_ps(&m[2][0]),
            _mm_load_ps(&m[3][0])};
#else
        return {m[0], m[1], m[2], m[3]};
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

    static inline void store(const matrix4& src, float4x3& dst)
    {
        store(src[0], dst[0]);
        store(src[1], dst[1]);
        store(src[2], dst[2]);
    }

    static inline void store(const matrix4& src, float4x3_align& dst)
    {
        store(src[0], dst[0]);
        store(src[1], dst[1]);
        store(src[2], dst[2]);
    }

    static inline void store(const matrix4& src, float4x4& dst)
    {
        store(src[0], dst[0]);
        store(src[1], dst[1]);
        store(src[2], dst[2]);
        store(src[3], dst[3]);
    }

    static inline void store(const matrix4& src, float4x4_align& dst)
    {
        store(src[0], dst[0]);
        store(src[1], dst[1]);
        store(src[2], dst[2]);
        store(src[3], dst[3]);
    }

    template <typename T>
    [[nodiscard]] static inline T store(matrix4 m)
    {
        T result;
        store(m, result);
        return result;
    }

public:
    [[nodiscard]] static inline float to_radians(float degrees)
    {
        return degrees * math::DEG_TO_RAD;
    }

    [[nodiscard]] static inline float to_degrees(float radians)
    {
        return radians * math::RAD_TO_DEG;
    }

    [[nodiscard]] static inline std::pair<float, float> sin_cos(float radians)
    {
        float temp = radians * math::INV_TWO_PI;
        if (temp > 0.0f)
            temp = static_cast<float>(static_cast<int>(temp + 0.5f));
        else
            temp = static_cast<float>(static_cast<int>(temp - 0.5f));

        float x = radians - math::TWO_PI * temp;

        float sign = 1.0f;
        if (x > math::HALF_PI)
        {
            x = math::PI - x;
            sign = -1.0f;
        }
        else if (x < -math::HALF_PI)
        {
            x = -math::PI - x;
            sign = -1.0f;
        }

        float x2 = x * x;

        float sin = (((((-2.3889859e-08f * x2 + 2.7525562e-06f) * x2 - 0.00019840874f) * x2 +
                       0.0083333310f) *
                          x2 -
                      0.16666667f) *
                         x2 +
                     1.0f) *
                    x;

        float cos =
            ((((-2.6051615e-07f * x2 + 2.4760495e-05f) * x2 - 0.0013888378f) * x2 + 0.041666638f) *
                 x2 -
             0.5f) *
                x2 +
            1.0f;

        return {sin, cos * sign};
    }

    [[nodiscard]] static inline float clamp(float value, float min, float max)
    {
        if (value < min)
            return min;
        else if (value > max)
            return max;
        else
            return value;
    }
};
} // namespace violet