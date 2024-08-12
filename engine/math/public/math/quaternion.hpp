#pragma once

#include "math/utility.hpp"
#include "math/vector.hpp"

namespace violet
{
class quaternion
{
public:
    [[nodiscard]] static inline vector4 identity()
    {
#ifdef VIOLET_USE_SIMD
        static constexpr vector4 identity = {0.0f, 0.0f, 0.0f, 1.0f};
        return identity;
#else
        return {0.0f, 0.0f, 0.0f, 1.0f};
#endif
    }

    [[nodiscard]] static inline vector4 from_axis_angle(vector4 axis, float radians)
    {
#ifdef VIOLET_USE_SIMD
        // TODO
        float4 a = math::store<float4>(axis);
        auto [sin, cos] = math::sin_cos(radians * 0.5f);
        return vector::set(a[0] * sin, a[1] * sin, a[2] * sin, cos);
#else
        auto [sin, cos] = sin_cos(radians * 0.5f);
        return {axis[0] * sin, axis[1] * sin, axis[2] * sin, cos};
#endif
    }

    [[nodiscard]] static inline vector4 from_euler(float pitch, float heading, float bank)
    {
#ifdef VIOLET_USE_SIMD
        // TODO
        auto [p_sin, p_cos] = math::sin_cos(pitch * 0.5f);
        auto [h_sin, h_cos] = math::sin_cos(heading * 0.5f);
        auto [b_sin, b_cos] = math::sin_cos(bank * 0.5f);

        return vector::set(
            h_cos * p_sin * b_cos + h_sin * p_cos * b_sin,
            h_sin * p_cos * b_cos - h_cos * p_sin * b_sin,
            h_cos * p_cos * b_sin - h_sin * p_sin * b_cos,
            h_cos * p_cos * b_cos + h_sin * p_sin * b_sin);
#else
        auto [p_sin, p_cos] = sin_cos(pitch * 0.5f);
        auto [h_sin, h_cos] = sin_cos(heading * 0.5f);
        auto [b_sin, b_cos] = sin_cos(bank * 0.5f);

        return {
            h_cos * p_sin * b_cos + h_sin * p_cos * b_sin,
            h_sin * p_cos * b_cos - h_cos * p_sin * b_sin,
            h_cos * p_cos * b_sin - h_sin * p_sin * b_cos,
            h_cos * p_cos * b_cos + h_sin * p_sin * b_sin};
#endif
    }

    [[nodiscard]] static inline vector4 from_matrix(matrix4 m)
    {
        // TODO
        float4x4 matrix = math::store<float4x4>(m);

        vector4 v;
        float t;

        if (matrix[2][2] < 0.0f) // x^2 + y ^2 > z^2 + w^2
        {
            if (matrix[0][0] > matrix[1][1]) // x > y
            {
                t = 1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2];
                v = vector::set(
                    t,
                    matrix[0][1] + matrix[1][0],
                    matrix[2][0] + matrix[0][2],
                    matrix[1][2] - matrix[2][1]);
            }
            else
            {
                t = 1.0f - matrix[0][0] + matrix[1][1] - matrix[2][2];
                v = vector::set(
                    matrix[0][1] + matrix[1][0],
                    t,
                    matrix[1][2] + matrix[2][1],
                    matrix[2][0] - matrix[0][2]);
            }
        }
        else
        {
            if (matrix[0][0] < -matrix[1][1]) // z > w
            {
                t = 1.0f - matrix[0][0] - matrix[1][1] + matrix[2][2];
                v = vector::set(
                    matrix[2][0] + matrix[0][2],
                    matrix[1][2] + matrix[2][1],
                    t,
                    matrix[0][1] - matrix[1][0]);
            }
            else
            {
                t = 1.0f + matrix[0][0] + matrix[1][1] + matrix[2][2];
                v = vector::set(
                    matrix[1][2] - matrix[2][1],
                    matrix[2][0] - matrix[0][2],
                    matrix[0][1] - matrix[1][0],
                    t);
            }
        }

        return vector::mul(v, 0.5f / sqrtf(t));
    }

    [[nodiscard]] static inline vector4 mul(vector4 a, const vector4 b)
    {
#ifdef VIOLET_USE_SIMD
        static const __m128 c1 = vector::set(1.0f, 1.0f, 1.0f, -1.0f);
        static const __m128 c2 = vector::set(-1.0f, -1.0f, -1.0f, -1.0f);

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
#else
        return {
            a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
            a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
            a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
            a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
#endif
    }

    [[nodiscard]] static inline vector4 mul_vec(vector4 q, vector4 v)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_and_ps(v, simd::mask_v<1, 1, 1, 0>);
        __m128 t2 = conjugate(q);
        t2 = mul(t1, t2);
        return mul(q, t2);
#else
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

        return {
            v[0] * (xxd + wwd - 1.0f) + v[1] * (xyd - zwd) + v[2] * (xzd + ywd),
            v[0] * (xyd + zwd) + v[1] * (yyd + wwd - 1.0f) + v[2] * (yzd - xwd),
            v[0] * (xzd - ywd) + v[1] * (yzd + xwd) + v[2] * (zzd + wwd - 1.0f),
            0.0f};
#endif
    }

    [[nodiscard]] static inline vector4 conjugate(vector4 q)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = vector::set(-1.0f, -1.0f, -1.0f, 1.0f);
        return _mm_mul_ps(q, t1);
#else
        return {-q[0], -q[1], -q[2], q[3]};
#endif
    }

    [[nodiscard]] static inline vector4 inverse(vector4 q)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = conjugate(q);
        __m128 t2 = vector::dot_v(q, q);
        return vector::div(t1, t2);
#else
        return vector::mul(conjugate(q), 1.0f / vector::dot(q, q));
#endif
    }

    [[nodiscard]] static inline vector4 slerp(vector4 a, vector4 b, float t)
    {
#ifdef VIOLET_USE_SIMD
        float cos_omega = vector::dot(a, b);

        if (cos_omega < 0.0f)
        {
            b = vector::mul(b, -1.0f);
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

        __m128 t1 = vector::mul(a, k0);
        __m128 t2 = vector::mul(b, k1);

        return _mm_add_ps(t1, t2);
#else
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
#endif
    }
};
} // namespace violet