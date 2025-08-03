#pragma once

#include "math/vector.hpp"

namespace violet
{
class quaternion
{
public:
    template <typename T>
    [[nodiscard]] static vec4<T> identity()
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            return _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            using value_type = vec4<T>::value_type;
            return {value_type(0), value_type(0), value_type(0), value_type(1)};
        }
    }

    template <typename T>
    [[nodiscard]] static vec4<T> from_axis_angle(const vec3<T>& axis, float radians)
    {
        auto [sin, cos] = math::sin_cos(radians * 0.5f);
        return {axis[0] * sin, axis[1] * sin, axis[2] * sin, cos};
    }

    [[nodiscard]] static vec4f_simd from_axis_angle(vec4f_simd axis, float radians)
    {
        // TODO
        vec4f a;
        math::store(axis, a);
        auto [sin, cos] = math::sin_cos(radians * 0.5f);
        return _mm_setr_ps(a[0] * sin, a[1] * sin, a[2] * sin, cos);
    }

    template <typename T>
    [[nodiscard]] static vec4<T> from_euler(const vec3<T>& euler)
    {
        auto [p_sin, p_cos] = math::sin_cos(euler.x * 0.5f);
        auto [h_sin, h_cos] = math::sin_cos(euler.y * 0.5f);
        auto [b_sin, b_cos] = math::sin_cos(euler.z * 0.5f);

        return {
            h_cos * p_sin * b_cos + h_sin * p_cos * b_sin,
            h_sin * p_cos * b_cos - h_cos * p_sin * b_sin,
            h_cos * p_cos * b_sin - h_sin * p_sin * b_cos,
            h_cos * p_cos * b_cos + h_sin * p_sin * b_sin};
    }

    [[nodiscard]] static vec4f_simd from_euler(vec4f_simd euler)
    {
        __m128 half_euler = vector::mul(euler, 0.5f);

        // TODO
        auto [p_sin, p_cos] = math::sin_cos(vector::get_x(half_euler));
        auto [h_sin, h_cos] = math::sin_cos(vector::get_y(half_euler));
        auto [b_sin, b_cos] = math::sin_cos(vector::get_z(half_euler));

        return vector::set(
            (h_cos * p_sin * b_cos) + (h_sin * p_cos * b_sin),
            (h_sin * p_cos * b_cos) - (h_cos * p_sin * b_sin),
            (h_cos * p_cos * b_sin) - (h_sin * p_sin * b_cos),
            (h_cos * p_cos * b_cos) + (h_sin * p_sin * b_sin));
    }

    template <typename T>
    [[nodiscard]] static vec4<T> from_matrix(const mat4<T> m)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            // TODO
            mat4f temp;
            math::store(m, temp);
            return math::load(from_matrix(temp));
        }
        else
        {
            vec4<T> v;
            float t;

            if (m[2][2] < 0.0f) // x^2 + y ^2 > z^2 + w^2
            {
                if (m[0][0] > m[1][1]) // x > y
                {
                    t = 1.0f + m[0][0] - m[1][1] - m[2][2];
                    v = {t, m[0][1] + m[1][0], m[2][0] + m[0][2], m[1][2] - m[2][1]};
                }
                else
                {
                    t = 1.0f - m[0][0] + m[1][1] - m[2][2];
                    v = {m[0][1] + m[1][0], t, m[1][2] + m[2][1], m[2][0] - m[0][2]};
                }
            }
            else
            {
                if (m[0][0] < -m[1][1]) // z > w
                {
                    t = 1.0f - m[0][0] - m[1][1] + m[2][2];
                    v = {m[2][0] + m[0][2], m[1][2] + m[2][1], t, m[0][1] - m[1][0]};
                }
                else
                {
                    t = 1.0f + m[0][0] + m[1][1] + m[2][2];
                    v = {m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0], t};
                }
            }

            return vector::mul(v, 0.5f / sqrtf(t));
        }
    }

    template <typename T>
    [[nodiscard]] static vec4<T> mul(const vec4<T>& a, const vec4<T>& b)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            static const __m128 c1 = vector::set(1.0f, 1.0f, 1.0f, -1.0f);
            static const __m128 c2 = vector::set(-1.0f, -1.0f, -1.0f, -1.0f);

            __m128 result;
            __m128 t1;
            __m128 t2;
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
        else
        {
            return {
                a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1],
                a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0],
                a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3],
                a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2]};
        }
    }

    template <typename T>
    [[nodiscard]] static vec4<T> mul_vec(const vec4<T>& q, const vec4<T>& v)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            vec4f_simd t1 = _mm_and_ps(v, simd::mask_v<1, 1, 1, 0>);
            vec4f_simd t2 = conjugate(q);
            vec4f_simd result = quaternion::mul(t1, t2);
            return quaternion::mul(q, result);
        }
        else
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

            return {
                v[0] * (xxd + wwd - 1.0f) + v[1] * (xyd - zwd) + v[2] * (xzd + ywd),
                v[0] * (xyd + zwd) + v[1] * (yyd + wwd - 1.0f) + v[2] * (yzd - xwd),
                v[0] * (xzd - ywd) + v[1] * (yzd + xwd) + v[2] * (zzd + wwd - 1.0f),
                0.0f};
        }
    }

    template <typename T>
    [[nodiscard]] static vec4<T> conjugate(const vec4<T>& q)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 t1 = vector::set(-1.0f, -1.0f, -1.0f, 1.0f);
            return _mm_mul_ps(q, t1);
        }
        else
        {
            return {-q[0], -q[1], -q[2], q[3]};
        }
    }

    template <typename T>
    [[nodiscard]] static vec4<T> inverse(const vec4<T>& q)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 t1 = conjugate(q);
            __m128 t2 = vector::dot_v(q, q);
            return vector::div(t1, t2);
        }
        else
        {
            return vector::mul(conjugate(q), 1.0f / vector::dot(q, q));
        }
    }

    template <typename T>
    [[nodiscard]] static vec4<T> slerp(const vec4<T>& a, const vec4<T>& b, vec4<T>::value_type t)
    {
        using value_type = vec4<T>::value_type;

        value_type cos_omega = vector::dot(a, b);

        vec4<T> c = b;
        if (cos_omega < 0.0f)
        {
            c = vector::mul(b, -1.0f);
            cos_omega = -cos_omega;
        }

        value_type k0;
        value_type k1;
        if (cos_omega > 0.9999f)
        {
            k0 = 1.0f - t;
            k1 = t;
        }
        else
        {
            value_type sin_omega = sqrtf(1.0f - cos_omega * cos_omega);
            value_type omega = atan2f(sin_omega, cos_omega);
            value_type div = 1.0f / sin_omega;
            k0 = sinf((1.0f - t) * omega) * div;
            k1 = sinf(t * omega) * div;
        }

        return {
            a[0] * k0 + c[0] * k1,
            a[1] * k0 + c[1] * k1,
            a[2] * k0 + c[2] * k1,
            a[3] * k0 + c[3] * k1};
    }

    [[nodiscard]] static vec4f_simd slerp(vec4f_simd a, vec4f_simd b, float t)
    {
        float cos_omega = vector::dot(a, b);

        if (cos_omega < 0.0f)
        {
            b = vector::mul(b, -1.0f);
            cos_omega = -cos_omega;
        }

        float k0;
        float k1;
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
    }
};
} // namespace violet