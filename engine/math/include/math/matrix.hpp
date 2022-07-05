#pragma once

#include "misc.hpp"
#include "quaternion.hpp"
#include "simd.hpp"
#include "type.hpp"
#include "vector.hpp"

namespace ash::math
{
class matrix_plain
{
public:
    static inline float4x4 mul(const float4x4& m1, const float4x4& m2)
    {
        float4x4 result = {};
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
            {
                for (std::size_t k = 0; k < 4; ++k)
                    result[i][j] += m1[i][k] * m2[k][j];
            }
        }
        return result;
    }

    static inline float4 mul(const float4& v, const float4x4& m)
    {
        return {
            m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3],
            m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3],
            m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3],
            m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3]};
    }

    static inline float4x4 mul(const float4x4& m, float scale)
    {
        float4x4 result = {};
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
                result[i][j] = m[i][j] * scale;
        }
        return result;
    }

    static inline float4x4 transpose(const float4x4& m)
    {
        float4x4 result = {};
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
                result[j][i] = m[i][j];
        }
        return result;
    }

    static inline float determinant(const float4x4& m)
    {
        float det11 = m[0][0] * (m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                                 m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) +
                                 m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]));
        float det12 = m[0][1] * (m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
                                 m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                                 m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]));
        float det13 = m[0][2] * (m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1]) -
                                 m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0]) +
                                 m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]));
        float det14 = m[0][3] * (m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]) -
                                 m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]) +
                                 m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]));

        return det11 - det12 + det13 - det14;
    }

    static inline float4x4 inverse(const float4x4& m)
    {
        float4x4 result = {};

        float det =
            (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
             m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
             m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]));

        float det_inv = 1.0f / det;

        result[0][0] = det_inv * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
        result[0][1] = -det_inv * (m[0][1] * m[2][2] - m[0][2] * m[2][1]);
        result[0][2] = det_inv * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
        result[0][3] = 0.0;

        result[1][0] = -det_inv * (m[1][0] * m[2][2] - m[1][2] * m[2][0]);
        result[1][1] = det_inv * (m[0][0] * m[2][2] - m[0][2] * m[2][0]);
        result[1][2] = -det_inv * (m[0][0] * m[1][2] - m[0][2] * m[1][0]);
        result[1][3] = 0.0;

        result[2][0] = det_inv * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
        result[2][1] = -det_inv * (m[0][0] * m[2][1] - m[0][1] * m[2][0]);
        result[2][2] = det_inv * (m[0][0] * m[1][1] - m[0][1] * m[1][0]);
        result[2][3] = 0.0;

        result[3][0] = -(m[3][0] * result[0][0] + m[3][1] * result[1][0] + m[3][2] * result[2][0]);
        result[3][1] = -(m[3][0] * result[0][1] + m[3][1] * result[1][1] + m[3][2] * result[2][1]);
        result[3][2] = -(m[3][0] * result[0][2] + m[3][1] * result[1][2] + m[3][2] * result[2][2]);
        result[3][3] = 1.0;

        return result;
    }

    static constexpr inline float4x4 identity()
    {
        return float4x4{
            float4{1.0f, 0.0f, 0.0f, 0.0f},
            float4{0.0f, 1.0f, 0.0f, 0.0f},
            float4{0.0f, 0.0f, 1.0f, 0.0f},
            float4{0.0f, 0.0f, 0.0f, 1.0f}
        };
    }

    static inline float4x4 scale(float x, float y, float z)
    {
        return float4x4{
            float4{x,    0.0f, 0.0f, 0.0f},
            float4{0.0f, y,    0.0f, 0.0f},
            float4{0.0f, 0.0f, z,    0.0f},
            float4{0.0f, 0.0f, 0.0f, 1.0f},
        };
    }

    static inline float4x4 scale(const float3& v) { return scale(v[0], v[1], v[2]); }
    static inline float4x4 scale(const float4& v) { return scale(v[0], v[1], v[2]); }

    static inline float4x4 scale_axis(const float4& axis, float scale)
    {
        float x2 = axis[0] * axis[0];
        float xy = axis[0] * axis[1];
        float xz = axis[0] * axis[2];
        float y2 = axis[1] * axis[1];
        float yz = axis[1] * axis[2];
        float z2 = axis[2] * axis[2];

        float4x4 result = identity();
        result[0][0] = 1.0f + (scale - 1.0f) * x2;
        result[0][1] = (scale - 1.0f) * xy;
        result[0][2] = (scale - 1.0f) * xz;

        result[1][0] = (scale - 1.0f) * xy;
        result[1][1] = (scale - 1.0f) * y2;
        result[1][2] = (scale - 1.0f) * yz;

        result[2][0] = (scale - 1.0f) * xz;
        result[2][1] = (scale - 1.0f) * yz;
        result[2][2] = 1.0f + (scale - 1.0f) * z2;

        return result;
    }

    static inline float4x4 rotation_axis(const float4& axis, float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        float x2 = axis[0] * axis[0];
        float xy = axis[0] * axis[1];
        float xz = axis[0] * axis[2];
        float y2 = axis[1] * axis[1];
        float yz = axis[1] * axis[2];
        float z2 = axis[2] * axis[2];

        float4x4 result = identity();
        result[0][0] = x2 * (1.0f - cos) + cos;
        result[0][1] = xy * (1.0f - cos) + axis[2] * sin;
        result[0][2] = xz * (1.0f - cos) - axis[1] * sin;

        result[1][0] = xy * (1.0f - cos) - axis[2] * sin;
        result[1][1] = y2 * (1.0f - cos) + cos;
        result[1][2] = yz * (1.0f - cos) + axis[0] * sin;

        result[2][0] = xz * (1.0f - cos) + axis[1] * sin;
        result[2][1] = yz * (1.0f - cos) - axis[0] * sin;
        result[2][2] = z2 * (1.0f - cos) + cos;

        return result;
    }

    static inline float4x4 rotation_x_axis(float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        float4x4 result = identity();
        result[1][1] = cos;
        result[1][2] = sin;
        result[2][1] = -sin;
        result[2][2] = cos;

        return result;
    }

    static inline float4x4 rotation_y_axis(float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        float4x4 result = identity();
        result[0][0] = cos;
        result[0][2] = -sin;
        result[2][0] = sin;
        result[2][2] = cos;

        return result;
    }

    static inline float4x4 rotation_z_axis(float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        float4x4 result = identity();
        result[0][0] = cos;
        result[0][1] = sin;
        result[1][0] = -sin;
        result[1][1] = cos;

        return result;
    }

    static inline float4x4 rotation_quaternion(const float4& quaternion)
    {
        float xxd = 2.0f * quaternion[0] * quaternion[0];
        float xyd = 2.0f * quaternion[0] * quaternion[1];
        float xzd = 2.0f * quaternion[0] * quaternion[2];
        float xwd = 2.0f * quaternion[0] * quaternion[3];
        float yyd = 2.0f * quaternion[1] * quaternion[1];
        float yzd = 2.0f * quaternion[1] * quaternion[2];
        float ywd = 2.0f * quaternion[1] * quaternion[3];
        float zzd = 2.0f * quaternion[2] * quaternion[2];
        float zwd = 2.0f * quaternion[2] * quaternion[3];
        float wwd = 2.0f * quaternion[3] * quaternion[3];

        return float4x4{
            float4{1.0f - yyd - zzd, xyd + zwd,        xzd - ywd,        0.0f},
            float4{xyd - zwd,        1.0f - xxd - zzd, yzd + xwd,        0.0f},
            float4{xzd + ywd,        yzd - xwd,        1.0f - xxd - yyd, 0.0f},
            float4{0.0f,             0.0f,             0.0f,             1.0f}
        };
    }

    static inline float4x4 affine_transform(
        const float3& scale,
        const float4& rotation,
        const float3& translation)
    {
        float4x4 r = rotation_quaternion(rotation);

        return float4x4{
            float4{scale[0] * r[0][0], scale[0] * r[0][1], scale[0] * r[0][2], 0.0f},
            float4{scale[1] * r[1][0], scale[1] * r[1][1], scale[1] * r[1][2], 0.0f},
            float4{scale[2] * r[2][0], scale[2] * r[2][1], scale[2] * r[2][2], 0.0f},
            float4{translation[0],     translation[1],     translation[2],     1.0f}
        };
    }

    static inline float4x4 affine_transform(
        const float4& scale,
        const float4& rotation,
        const float4& translation)
    {
        float4x4 r = rotation_quaternion(rotation);

        return float4x4{
            float4{scale[0] * r[0][0], scale[0] * r[0][1], scale[0] * r[0][2], 0.0f},
            float4{scale[1] * r[1][0], scale[1] * r[1][1], scale[1] * r[1][2], 0.0f},
            float4{scale[2] * r[2][0], scale[2] * r[2][1], scale[2] * r[2][2], 0.0f},
            float4{translation[0],     translation[1],     translation[2],     1.0f}
        };
    }

    static inline void decompose(
        const float4x4& m,
        float3& scale,
        float4& rotation,
        float3& translation)
    {
        scale[0] = vector_plain::length(m[0]);
        scale[1] = vector_plain::length(m[1]);
        scale[2] = vector_plain::length(m[2]);

        float4x4 r = {
            vector_plain::scale(m[0], 1.0f / scale[0]),
            vector_plain::scale(m[1], 1.0f / scale[1]),
            vector_plain::scale(m[2], 1.0f / scale[2]),
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        rotation = quaternion_plain::rotation_matrix(r);

        translation = {m[3][0], m[3][1], m[3][2]};
    }

    static inline void decompose(
        const float4x4& m,
        float4& scale,
        float4& rotation,
        float4& translation)
    {
        scale[0] = vector_plain::length(m[0]);
        scale[1] = vector_plain::length(m[1]);
        scale[2] = vector_plain::length(m[2]);

        float4x4 r = {
            vector_plain::scale(m[0], 1.0f / scale[0]),
            vector_plain::scale(m[1], 1.0f / scale[1]),
            vector_plain::scale(m[2], 1.0f / scale[2]),
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        rotation = quaternion_plain::rotation_matrix(r);

        translation = m[3];
    }

    static inline float4x4 orthographic(
        float left,
        float right,
        float bottom,
        float top,
        float near_z,
        float far_z)
    {
        float w = 1.0f / (right - left);
        float h = 1.0f / (top - bottom);
        float d = 1.0f / (far_z - near_z);
        return float4x4{
            float4{w + w,               0.0f,                0.0f,        0.0f},
            float4{0.0f,                h + h,               0.0f,        0.0f},
            float4{0.0f,                0,                   d,           0.0f},
            float4{(left + right) * -w, (bottom + top) * -h, near_z * -d, 1.0f}
        };
    }

    static inline float4x4 orthographic(float width, float height, float near_z, float far_z)
    {
        float d = 1.0f / (far_z - near_z);
        return float4x4{
            float4{2.0f / width, 0.0f,          0.0f,        0.0f},
            float4{0.0f,         2.0f / height, 0.0f,        0.0f},
            float4{0.0f,         0.0f,          d,           0.0f},
            float4{0.0f,         0.0f,          near_z * -d, 1.0f}
        };
    }

    static inline float4x4 perspective(float fov, float aspect, float zn, float zf)
    {
        float h = 1.0f / tanf(fov * 0.5f); // view space height
        float w = h / aspect;              // view space width
        return float4x4{
            float4{w,    0.0f, 0.0f,                0.0f},
            float4{0.0f, h,    0.0f,                0.0f},
            float4{0.0f, 0.0f, zf / (zf - zn),      1.0f},
            float4{0.0f, 0.0f, zn * zf / (zn - zf), 0.0f}
        };
    }
};

class matrix_simd
{
public:
    static inline float4x4_simd mul(const float4x4_simd& m1, const float4x4_simd& m2)
    {
        __m128 temp;
        float4x4_simd result;

        // row 1
        temp = _mm_mul_ps(simd::replicate<0>(m1[0]), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1[0]), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1[0]), m2[2]), temp);
        result[0] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1[0]), m2[3]), temp);

        // row 2
        temp = _mm_mul_ps(simd::replicate<0>(m1[1]), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1[1]), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1[1]), m2[2]), temp);
        result[1] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1[1]), m2[3]), temp);

        // row 3
        temp = _mm_mul_ps(simd::replicate<0>(m1[2]), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1[2]), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1[2]), m2[2]), temp);
        result[2] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1[2]), m2[3]), temp);

        // row 4
        temp = _mm_mul_ps(simd::replicate<0>(m1[3]), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1[3]), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1[3]), m2[2]), temp);
        result[3] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1[3]), m2[3]), temp);

        return result;
    }

    static inline float4_simd mul(const float4_simd& v, const float4x4_simd& m)
    {
        __m128 t = simd::replicate<0>(v);
        t = _mm_mul_ps(t, m[0]);
        __m128 result = t;

        t = simd::replicate<1>(v);
        t = _mm_mul_ps(t, m[1]);
        result = _mm_add_ps(result, t);

        t = simd::replicate<2>(v);
        t = _mm_mul_ps(t, m[2]);
        result = _mm_add_ps(result, t);

        t = simd::replicate<3>(v);
        t = _mm_mul_ps(t, m[3]);
        result = _mm_add_ps(result, t);

        return result;
    }

    static inline float4x4_simd scale(const float4x4_simd& m, float scale)
    {
        float4x4_simd result;
        __m128 s = _mm_set_ps1(scale);

        result[0] = _mm_mul_ps(m[0], s);
        result[1] = _mm_mul_ps(m[1], s);
        result[2] = _mm_mul_ps(m[2], s);
        result[3] = _mm_mul_ps(m[3], s);

        return result;
    }

    static inline float4x4_simd transpose(const float4x4_simd& m)
    {
        __m128 t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]);
        __m128 t2 = simd::shuffle<0, 1, 0, 1>(m[2], m[3]);
        __m128 t3 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]);
        __m128 t4 = simd::shuffle<2, 3, 2, 3>(m[2], m[3]);

        float4x4_simd result;
        result[0] = simd::shuffle<0, 2, 0, 2>(t1, t2);
        result[1] = simd::shuffle<1, 3, 1, 3>(t1, t2);
        result[2] = simd::shuffle<0, 2, 0, 2>(t3, t4);
        result[3] = simd::shuffle<1, 3, 1, 3>(t3, t4);

        return result;
    }

    static inline float determinant(const float4x4_simd& m)
    {
        // TODO
    }

    static inline float4x4_simd inverse(const float4x4_simd& m)
    {
        // TODO
        float4x4 result;
        simd::store(m, result);
        result = matrix_plain::inverse(result);
        return simd::load(result);
    }

    static inline float4x4_simd identity()
    {
        return float4x4_simd{
            simd::identity_row<0>(),
            simd::identity_row<1>(),
            simd::identity_row<2>(),
            simd::identity_row<3>()};
    }

    static inline float4x4_simd scale(float x, float y, float z)
    {
        return float4x4_simd{
            _mm_setr_ps(x, 0.0f, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, y, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, 0.0f, z, 0.0f),
            simd::identity_row<3>()};
    }

    static inline float4x4_simd scale(const float4_simd& v)
    {
        return float4x4_simd{
            _mm_and_ps(v, simd::mask<0x1000>()),
            _mm_and_ps(v, simd::mask<0x0100>()),
            _mm_and_ps(v, simd::mask<0x0010>()),
            simd::identity_row<3>()};
    }

    static inline float4x4_simd scale_axis(const float4_simd& axis, float scale)
    {
        // TODO
    }

    static inline float4x4_simd rotation_axis(const float4_simd& axis, float radians)
    {
        // TODO
    }

    static inline float4x4_simd rotation_x_axis(float radians)
    {
        // TODO
    }

    static inline float4x4_simd rotation_y_axis(float radians)
    {
        // TODO
    }

    static inline float4x4_simd rotation_z_axis(float radians)
    {
        // TODO
    }

    static inline float4x4_simd rotation_quaternion(const float4_simd& quaternion)
    {
        const __m128 c = simd::set(1.0f, 1.0f, 1.0f, 0.0f);

        __m128 q0 = _mm_add_ps(quaternion, quaternion);
        __m128 q1 = _mm_mul_ps(quaternion, q0);

        __m128 v0 = simd::shuffle<1, 0, 0, 3>(q1);
        v0 = _mm_and_ps(v0, simd::mask<0x1110>());
        __m128 v1 = simd::shuffle<2, 2, 1, 3>(q1);
        v1 = _mm_and_ps(v1, simd::mask<0x1110>());
        __m128 r0 = _mm_sub_ps(c, v0);
        r0 = _mm_sub_ps(r0, v1);

        v0 = simd::shuffle<0, 0, 1, 3>(quaternion);
        v1 = simd::shuffle<2, 1, 2, 3>(q0);
        v0 = _mm_mul_ps(v0, v1);

        v1 = simd::replicate<3>(quaternion);
        __m128 v2 = simd::shuffle<1, 2, 0, 3>(q0);
        v1 = _mm_mul_ps(v1, v2);

        __m128 r1 = _mm_add_ps(v0, v1);
        __m128 r2 = _mm_sub_ps(v0, v1);

        v0 = simd::shuffle<1, 2, 0, 1>(r1, r2);
        v0 = simd::shuffle<0, 2, 3, 1>(v0);
        v1 = simd::shuffle<0, 0, 2, 2>(r1, r2);
        v1 = simd::shuffle<0, 2, 0, 2>(v1);

        q1 = simd::shuffle<0, 3, 0, 1>(r0, v0);
        q1 = simd::shuffle<0, 2, 3, 1>(q1);

        float4x4_simd result =
            {q1, simd::identity_row<1>(), simd::identity_row<2>(), simd::identity_row<3>()};

        q1 = simd::shuffle<1, 3, 2, 3>(r0, v0);
        q1 = simd::shuffle<2, 0, 3, 1>(q1);
        result[1] = q1;

        q1 = simd::shuffle<0, 1, 2, 3>(v1, r0);
        result[2] = q1;

        return result;
    }

    static inline float4x4_simd affine_transform(
        const float4_simd& scale,
        const float4_simd& rotation,
        const float4_simd& translation)
    {
        float4x4_simd r = rotation_quaternion(rotation);
        __m128 t = _mm_and_ps(translation, simd::mask<0x1110>());

        __m128 s1 = simd::replicate<0>(scale);
        __m128 s2 = simd::replicate<1>(scale);
        __m128 s3 = simd::replicate<2>(scale);

        float4x4_simd result;
        result[0] = _mm_mul_ps(s1, r[0]);
        result[1] = _mm_mul_ps(s2, r[1]);
        result[2] = _mm_mul_ps(s3, r[2]);
        result[3] = _mm_add_ps(simd::identity_row<3>(), t);

        return result;
    }

    static inline void decompose(
        const float4x4_simd& m,
        float4_simd& scale,
        float4_simd& rotation,
        float4_simd& translation)
    {
        __m128 s0 = vector_simd::length_v(m[0]);
        __m128 s1 = vector_simd::length_v(m[1]);
        __m128 s2 = vector_simd::length_v(m[2]);

        scale = _mm_and_ps(simd::mask<0x1000>(), s0);
        scale = _mm_add_ps(scale, _mm_and_ps(simd::mask<0x0100>(), s1));
        scale = _mm_add_ps(scale, _mm_and_ps(simd::mask<0x0010>(), s2));

        float4x4_simd r = {
            _mm_div_ps(m[0], s0),
            _mm_div_ps(m[1], s1),
            _mm_div_ps(m[2], s2),
            simd::identity_row<3>()};
        rotation = quaternion_simd::rotation_matrix(r);

        translation = m[3];
    }

    static inline float4x4_simd perspective(float fov, float aspect, float zn, float zf)
    {
        // TODO
        float4x4 result = matrix_plain::perspective(fov, aspect, zn, zf);
        return simd::load(result);
    }
};
} // namespace ash::math