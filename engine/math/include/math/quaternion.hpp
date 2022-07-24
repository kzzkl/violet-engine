#pragma once

#include "misc.hpp"
#include "vector.hpp"

namespace ash::math
{
class quaternion
{
public:
    static constexpr inline float4 identity() { return {0.0f, 0.0f, 0.0f, 1.0f}; }

    static inline float4 rotation_axis(const float3& axis, float radians)
    {
        auto [sin, cos] = sin_cos(radians * 0.5f);
        return {axis[0] * sin, axis[1] * sin, axis[2] * sin, cos};
    }

    static inline float4 rotation_axis(const float4& axis, float radians)
    {
        auto [sin, cos] = sin_cos(radians * 0.5f);
        return {axis[0] * sin, axis[1] * sin, axis[2] * sin, cos};
    }

    static inline float4 rotation_euler(float pitch, float heading, float bank)
    {
        auto [p_sin, p_cos] = sin_cos(pitch * 0.5f);
        auto [h_sin, h_cos] = sin_cos(heading * 0.5f);
        auto [b_sin, b_cos] = sin_cos(bank * 0.5f);

        return float4{
            h_cos * p_sin * b_cos + h_sin * p_cos * b_sin,
            h_sin * p_cos * b_cos - h_cos * p_sin * b_sin,
            h_cos * p_cos * b_sin - h_sin * p_sin * b_cos,
            h_cos * p_cos * b_cos + h_sin * p_sin * b_sin};
    }

    static inline float4 rotation_euler(const float3& euler)
    {
        return rotation_euler(euler[0], euler[1], euler[2]);
    }

    static inline float4 rotation_euler(const float4& euler)
    {
        return rotation_euler(euler[0], euler[1], euler[2]);
    }

    static inline float4 rotation_matrix(const float4x4& m)
    {
        float4 result;
        float t;

        if (m[2][2] < 0.0f) // x^2 + y ^2 > z^2 + w^2
        {
            if (m[0][0] > m[1][1]) // x > y
            {
                t = 1.0f + m[0][0] - m[1][1] - m[2][2];
                result = {t, m[0][1] + m[1][0], m[2][0] + m[0][2], m[1][2] - m[2][1]};
            }
            else
            {
                t = 1.0f - m[0][0] + m[1][1] - m[2][2];
                result = {m[0][1] + m[1][0], t, m[1][2] + m[2][1], m[2][0] - m[0][2]};
            }
        }
        else
        {
            if (m[0][0] < -m[1][1]) // z > w
            {
                t = 1.0f - m[0][0] - m[1][1] + m[2][2];
                result = {m[2][0] + m[0][2], m[1][2] + m[2][1], t, m[0][1] - m[1][0]};
            }
            else
            {
                t = 1.0f + m[0][0] + m[1][1] + m[2][2];
                result = {m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0], t};
            }
        }

        result = vector::mul(result, 0.5f / sqrtf(t));
        return result;
    }

    static inline float4 mul(const float4& a, const float4& b)
    {
        return float4{
            a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
            a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
            a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
            a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
    }

    static inline float4 mul_vec(const float4& q, const float4& v)
    {
        float xxd = 2.0f * q[0] * q[0];
        float xyd = 2.0f * q[0] * q[1];
        float xzd = 2.0f * q[0] * q[2];
        float xwd = 2.0f * q[0] * q[3];
        float yyd = 2.0f * q[1] * q[1];
        float yzd = 2.0f * q[1] * q[2];
        float ywd = 2.0f * q[1] * q[3];
        float zzd = 2.0f * q[2] * q[2];
        float zwd = 2.0f * q[2] * q[3];
        float wwd = 2.0f * q[3] * q[3];

        return float4{
            v[0] * (xxd + wwd - 1.0f) + v[1] * (xyd - zwd) + v[2] * (xzd + ywd),
            v[0] * (xyd + zwd) + v[1] * (yyd + wwd - 1.0f) + v[2] * (yzd - xwd),
            v[0] * (xzd - ywd) + v[1] * (yzd + xwd) + v[2] * (zzd + wwd - 1.0f),
            0.0f};
    }

    static inline float4 conjugate(const float4& q) { return float4{-q[0], -q[1], -q[2], q[3]}; }

    static inline float4 inverse(const float4& q)
    {
        return vector::mul(conjugate(q), 1.0f / vector::dot(q, q));
    }

    static inline float4 slerp(const float4& a, const float4& b, float t)
    {
        float cos_omega = vector::dot(a, b);

        float4 c = b;
        if (cos_omega < 0.0f)
        {
            c = vector::mul(b, -1.0f);
            cos_omega = -cos_omega;
        }

        float k0, k1;
        if (cos_omega > 0.9999f)
        {
            k0 = 1.0f - t;
            k1 = t;
        }
        else
        {
            float sin_omega = sqrtf(1.0f - cos_omega * cos_omega);
            float omega = atan2f(sin_omega, cos_omega);
            float div = 1.0f / sin_omega;
            k0 = sinf((1.0f - t) * omega) * div;
            k1 = sinf(t * omega) * div;
        }

        return {
            a[0] * k0 + c[0] * k1,
            a[1] * k0 + c[1] * k1,
            a[2] * k0 + c[2] * k1,
            a[3] * k0 + c[3] * k1};
    }
};

struct quaternion_simd
{
public:
    static inline float4_simd identity() { return simd::identity_row_v<3>; }

    static inline float4_simd rotation_axis(float4_simd axis, float radians)
    {
        // TODO
        float4 a;
        simd::store(axis, a);
        float4 result = quaternion::rotation_axis(a, radians);
        return simd::load(result);
    }

    static inline float4_simd rotation_euler(float4_simd euler)
    {
        // TODO
        float4 e;
        simd::store(euler, e);
        float4 result = quaternion::rotation_euler(e);
        return simd::load(result);
    }

    static inline float4_simd rotation_matrix(const float4x4_simd& m)
    {
        // TODO
        float4x4 temp;
        simd::store(m, temp);
        float4 q = quaternion::rotation_matrix(temp);
        return simd::load(q);
    }

    static inline float4_simd mul(float4_simd a, float4_simd b)
    {
        static const __m128 c1 = simd::set(1.0f, 1.0f, 1.0f, -1.0f);
        static const __m128 c2 = simd::set(-1.0f, -1.0f, -1.0f, -1.0f);

        __m128 result, t1, t2;
        result = simd::replicate<3>(a); // [aw, aw, aw, aw]
        result = _mm_mul_ps(result, b); // [aw * bx, aw * by, aw * bz, aw * bw]

        t1 = simd::shuffle<0, 1, 2, 0>(a); // [ax, ay, az, ax]
        t2 = simd::shuffle<3, 3, 3, 0>(b); // [bw, bw, bw, bx]
        t1 = _mm_mul_ps(t1, t2);           // [ax * bw, ay * bw, az * bw, ax * bx]
        t1 = _mm_mul_ps(t1, c1);           // [ax * bw, ay * bw, az * bw, -ax * bx]
        result = _mm_add_ps(result, t1);   // [aw * bx + ax * bw,
                                           //  aw * by + ay * bw,
                                           //  aw * bz + az * bw,
                                           //  aw * bw - ax * bx]

        t1 = simd::shuffle<1, 2, 0, 1>(a); // [ay, az, ax, ay]
        t2 = simd::shuffle<2, 0, 1, 1>(b); // [bz, bx, by, by]
        t1 = _mm_mul_ps(t1, t2);           // [ay * bz, az * bx, ax * by, ay * by]
        t1 = _mm_mul_ps(t1, c1);           // [ay * bz, az * bx, ax * by, -ay * by]
        result = _mm_add_ps(result, t1);   // [aw * bx + ax * bw + ay * bz,
                                           //  aw * by + ay * bw + az * bx,
                                           //  aw * bz + az * bw + ax * by,
                                           //  aw * bw - ax * bx - ay * by]

        t1 = simd::shuffle<2, 0, 1, 2>(a); // [az, ax, ay, az]
        t2 = simd::shuffle<1, 2, 0, 2>(b); // [by, bz, bx, bz]
        t1 = _mm_mul_ps(t1, t2);           // [az * by, ax * bz, ay * bx, az * bz]
        t1 = _mm_mul_ps(t1, c2);           // [-az * by, -ax * bz, -ay * bx, -az * bz]
        result = _mm_add_ps(result, t1);   // [aw * bx + ax * bw + ay * bz - az * by,
                                           //  aw * by + ay * bw + az * bx - ax * bz,
                                           //  aw * bz + az * bw + ax * by - ay * bx,
                                           //  aw * bw - ax * bx - ay * by - az * bz]

        return result;
    }

    static inline float4_simd mul_vec(float4_simd q, float4_simd v)
    {
        __m128 t1 = _mm_and_ps(v, simd::mask_v<1, 1, 1, 0>);
        __m128 t2 = conjugate(q);
        t2 = mul(t1, t2);
        return mul(q, t2);
    }

    static inline float4_simd conjugate(float4_simd q)
    {
        __m128 t1 = simd::set(-1.0f, -1.0f, -1.0f, 1.0f);
        return _mm_mul_ps(q, t1);
    }

    static inline float4_simd inverse(float4_simd q)
    {
        __m128 t1 = conjugate(q);
        __m128 t2 = vector_simd::dot_v(q, q);
        return vector_simd::div(t1, t2);
    }

    static inline float4_simd slerp(float4_simd a, float4_simd b, float t)
    {
        float cos_omega = vector_simd::dot(a, b);

        if (cos_omega < 0.0f)
        {
            b = vector_simd::mul(b, -1.0f);
            cos_omega = -cos_omega;
        }

        float k0, k1;
        if (cos_omega > 0.9999f)
        {
            k0 = 1.0f - t;
            k1 = t;
        }
        else
        {
            float sin_omega = sqrtf(1.0f - cos_omega * cos_omega);
            float omega = atan2f(sin_omega, cos_omega);
            float div = 1.0f / sin_omega;
            k0 = sinf((1.0f - t) * omega) * div;
            k1 = sinf(t * omega) * div;
        }

        __m128 t1 = vector_simd::mul(a, k0);
        __m128 t2 = vector_simd::mul(b, k1);

        return _mm_add_ps(t1, t2);
    }
};
} // namespace ash::math