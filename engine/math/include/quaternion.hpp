#pragma once

#include "misc.hpp"
#include "vector.hpp"

namespace ash::math
{
class quaternion_plain
{
public:
    using quaternion_type = float4;
    using vector_type = float4;

public:
    static inline quaternion_type make_from_axis_angle(const vector_type& axis, float radians)
    {
        auto [sin, cos] = sin_cos(radians / 2.0f);
        return {axis[0] * sin, axis[1] * sin, axis[2] * sin, cos};
    }

    static inline quaternion_type make_from_euler(float heading, float pitch, float bank)
    {
        /*auto [roll_sin, roll_cos] = sin_cos(roll * T(0.5));
        auto [pitch_sin, pitch_cos] = sin_cos(pitch * T(0.5));
        auto [yaw_sin, yaw_cos] = sin_cos(yaw * T(0.5));

        return {yaw_cos * pitch_cos * roll_sin - yaw_sin * pitch_sin * roll_cos,
                yaw_sin * pitch_cos * roll_sin + yaw_cos * pitch_sin * roll_cos,
                yaw_sin * pitch_cos * roll_sin - yaw_cos * pitch_sin * roll_cos,
                yaw_cos * pitch_cos * roll_cos + yaw_sin * pitch_sin * roll_sin};*/

        // TODO
        return {};
    }

    static inline quaternion_type make_from_euler(const vector_type& euler)
    {
        return make_from_euler(euler[0], euler[1], euler[2]);
    }

    static inline quaternion_type mul(const quaternion_type& a, const quaternion_type& b)
    {
        return quaternion_type{
            a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
            a[3] * b[1] + a[1] * b[3] + a[2] * b[0] - a[0] * b[2],
            a[3] * b[2] + a[2] * b[3] + a[0] * b[1] - a[1] * b[0],
            a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
    }

    static inline float dot(const quaternion_type& a, const quaternion_type& b)
    {
        return vector_plain::dot(a, b);
    }

    static inline quaternion_type slerp(const quaternion_type& a, const quaternion_type& b, float t)
    {
    }
};

struct quaternion_simd
{
public:
    using quaternion_type = float4_simd;
    using vector_type = float4_simd;

public:
    static inline quaternion_type make_from_axis_angle(const vector_type& axis, float radians)
    {
        // TODO
    }

    static inline quaternion_type make_from_euler(const vector_type& euler)
    {
        // TODO
    }

    static inline quaternion_type mul(const quaternion_type& a, const quaternion_type& b)
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

        result = _mm_add_ps(result, t1); // [aw * bx + ax * bw,
                                         //  aw * by + ay * bw,
                                         //  aw * bz + az * bw,
                                         //  aw * bw - ax * bx]

        t1 = simd::shuffle<1, 2, 0, 1>(a); // [ay, az, ax, ay]
        t2 = simd::shuffle<2, 0, 1, 1>(b); // [bz, bx, by, by]
        t1 = _mm_mul_ps(t1, t2);           // [ay * bz, az * bx, ax * by, ay * by]
        t1 = _mm_mul_ps(t1, c1);           // [ay * bz, az * bx, ax * by, -ay * by]

        result = _mm_add_ps(result, t1); // [aw * bx + ax * bw + ay * bz,
                                         //  aw * by + ay * bw + az * bx,
                                         //  aw * bz + az * bw + ax * by,
                                         //  aw * bw - ax * bx - ay * by]

        t1 = simd::shuffle<2, 0, 1, 2>(a); // [az, ax, ay, az]
        t2 = simd::shuffle<1, 2, 0, 2>(b); // [by, bz, bx, bz]
        t1 = _mm_mul_ps(t1, t2);           // [az * by, ax * bz, ay * bx, az * bz]
        t1 = _mm_mul_ps(t1, c2);           // [-az * by, -ax * bz, -ay * bx, -az * bz]

        result = _mm_add_ps(result, t1); // [aw * bx + ax * bw + ay * bz - az * by,
                                         //  aw * by + ay * bw + az * bx - ax * bz,
                                         //  aw * bz + az * bw + ax * by - ay * bx,
                                         //  aw * bw - ax * bx - ay * by - az * bz]

        return result;
    }
};
} // namespace ash::math