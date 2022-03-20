#pragma once

#include "misc.hpp"
#include "simd.hpp"
#include "type.hpp"

namespace ash::math
{
class matrix_plain
{
public:
    using matrix_type = float4x4;
    using vector_type = matrix_type::row_type;

public:
    static inline matrix_type mul(const matrix_type& m1, const matrix_type& m2)
    {
        matrix_type result = {};
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

    static inline matrix_type scale(const matrix_type& m, float scale)
    {
        matrix_type result = {};
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
                result[i][j] = m[i][j] * scale;
        }
        return result;
    }

    static inline matrix_type transpose(const matrix_type& m)
    {
        matrix_type result = {};
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
                result[j][i] = m[i][j];
        }
        return result;
    }

    static inline float determinant(const matrix_type& m)
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

    static inline matrix_type inverse(const matrix_type& m)
    {
        matrix_type result = {};

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

    static inline matrix_type identity()
    {
        return matrix_type{
            vector_type{1.0f, 0.0f, 0.0f, 0.0f},
            vector_type{0.0f, 1.0f, 0.0f, 0.0f},
            vector_type{0.0f, 0.0f, 1.0f, 0.0f},
            vector_type{0.0f, 0.0f, 0.0f, 1.0f}
        };
    }

    static inline matrix_type perspective(float fov, float aspect, float zn, float zf)
    {
        float h = 1.0f / tanf(fov * 0.5f); // view space height
        float w = h / aspect;              // view space width
        return matrix_type{
            vector_type{w, 0, 0,                   0},
            vector_type{0, h, 0,                   0},
            vector_type{0, 0, zf / (zf - zn),      1},
            vector_type{0, 0, zn * zf / (zn - zf), 0}
        };
    }

    static inline matrix_type scaling(float x, float y, float z)
    {
        return matrix_type{
            vector_type{x,    0.0f, 0.0f, 0.0f},
            vector_type{0.0f, y,    0.0f, 0.0f},
            vector_type{0.0f, 0.0f, z,    0.0f},
            vector_type{0.0f, 0.0f, 0.0f, 1.0f},
        };
    }

    static inline matrix_type scaling(const vector_type& v) { return scaling(v[0], v[1], v[2]); }

    static inline matrix_type scaling_axis(const vector_type& axis, float scale)
    {
        float x2 = axis[0] * axis[0];
        float xy = axis[0] * axis[1];
        float xz = axis[0] * axis[2];
        float y2 = axis[1] * axis[1];
        float yz = axis[1] * axis[2];
        float z2 = axis[2] * axis[2];

        matrix_type result = identity();
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

    static inline matrix_type rotation_axis(const vector_type& axis, float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        float x2 = axis[0] * axis[0];
        float xy = axis[0] * axis[1];
        float xz = axis[0] * axis[2];
        float y2 = axis[1] * axis[1];
        float yz = axis[1] * axis[2];
        float z2 = axis[2] * axis[2];

        matrix_type result = identity();
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

    static inline matrix_type rotation_x_axis(float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        matrix_type result = identity();
        result[1][1] = cos;
        result[1][2] = sin;
        result[2][1] = -sin;
        result[2][2] = cos;

        return result;
    }

    static inline matrix_type rotation_y_axis(float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        matrix_type result = identity();
        result[0][0] = cos;
        result[0][2] = -sin;
        result[2][0] = sin;
        result[2][2] = cos;

        return result;
    }

    static inline matrix_type rotation_z_axis(float radians)
    {
        auto [sin, cos] = sin_cos(radians);

        matrix_type result = identity();
        result[0][0] = cos;
        result[0][1] = sin;
        result[1][0] = -sin;
        result[1][1] = cos;

        return result;
    }

    static inline matrix_type rotation_quaternion(const vector_type& quaternion)
    {
        // TODO
    }

    static inline matrix_type affine_transform(
        const vector_type& translation,
        const vector_type& rotation)
    {
        // TODO
    }

    static inline matrix_type affine_transform(
        const vector_type& translation,
        const vector_type& rotation,
        const vector_type& scale)
    {
        // TODO
    }
};

class matrix_simd
{
public:
    using matrix_type = float4x4_simd;
    using vector_type = matrix_type::row_type;

public:
    static inline matrix_type mul(const matrix_type& m1, const matrix_type& m2)
    {
        __m128 temp;
        matrix_type result;

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

    static inline matrix_type scale(const matrix_type& m, float scale)
    {
        matrix_type result;
        __m128 s = _mm_set_ps1(scale);

        result[0] = _mm_mul_ps(m[0], s);
        result[1] = _mm_mul_ps(m[1], s);
        result[2] = _mm_mul_ps(m[2], s);
        result[3] = _mm_mul_ps(m[3], s);

        return result;
    }

    static inline matrix_type transpose(const matrix_type& m)
    {
        __m128 t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]);
        __m128 t2 = simd::shuffle<0, 1, 0, 1>(m[2], m[3]);
        __m128 t3 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]);
        __m128 t4 = simd::shuffle<2, 3, 2, 3>(m[2], m[3]);

        matrix_type result;
        result[0] = simd::shuffle<0, 2, 0, 2>(t1, t2);
        result[1] = simd::shuffle<1, 3, 1, 3>(t1, t2);
        result[2] = simd::shuffle<0, 2, 0, 2>(t3, t4);
        result[3] = simd::shuffle<1, 3, 1, 3>(t3, t4);

        return result;
    }

    static inline float determinant(const matrix_type& m)
    {
        // TODO
    }

    static inline matrix_type inverse(const matrix_type& m)
    {
        // TODO
        return identity();
    }

    static inline matrix_type identity()
    {
        return matrix_type{
            simd::get_identity_row<0>(),
            simd::get_identity_row<1>(),
            simd::get_identity_row<2>(),
            simd::get_identity_row<3>()};
    }

    static inline matrix_type perspective(float fov, float aspect, float zn, float zf)
    {
        return identity();
    }

    static inline matrix_type scaling(float x, float y, float z)
    {
        return matrix_type{
            _mm_setr_ps(x, 0.0f, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, y, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, 0.0f, z, 0.0f),
            simd::get_identity_row<3>()};
    }

    static inline matrix_type scaling(const vector_type& v)
    {
        return matrix_type{
            _mm_and_ps(v, simd::get_mask<0x1000>()),
            _mm_and_ps(v, simd::get_mask<0x0100>()),
            _mm_and_ps(v, simd::get_mask<0x0010>()),
            simd::get_identity_row<3>()};
    }

    static inline matrix_type scaling_axis(const vector_type& axis, float scale)
    {
        // TODO
    }

    static inline matrix_type rotation_axis(const vector_type& axis, float radians)
    {
        // TODO
    }

    static inline matrix_type rotation_x_axis(float radians)
    {
        // TODO
    }

    static inline matrix_type rotation_y_axis(float radians)
    {
        // TODO
    }

    static inline matrix_type rotation_z_axis(float radians)
    {
        // TODO
    }

    static inline matrix_type rotation_quaternion(const vector_type& quaternion) noexcept
    {
        const __m128 c = simd::set(1.0f, 1.0f, 1.0f, 0.0f);

        __m128 q0 = _mm_add_ps(quaternion, quaternion);
        __m128 q1 = _mm_mul_ps(quaternion, q0);

        __m128 v0 = simd::shuffle<1, 0, 0, 3>(q1);
        v0 = _mm_and_ps(v0, simd::get_mask<0x1110>());
        __m128 v1 = simd::shuffle<2, 2, 1, 3>(q1);
        v1 = _mm_and_ps(v1, simd::get_mask<0x1110>());
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

        matrix_type result = {
            q1,
            simd::get_identity_row<1>(),
            simd::get_identity_row<2>(),
            simd::get_identity_row<3>()};

        q1 = simd::shuffle<1, 3, 2, 3>(r0, v0);
        q1 = simd::shuffle<2, 0, 3, 1>(q1);
        result[1] = q1;

        q1 = simd::shuffle<0, 1, 2, 3>(v1, r0);
        result[2] = q1;

        return result;
    }

    static inline matrix_type affine_transform(
        const vector_type& translation,
        const vector_type& rotation) noexcept
    {
        matrix_type result = rotation_quaternion(rotation);
        result[3] = _mm_or_ps(result[3], translation);
        return result;
    }

    static inline matrix_type affine_transform(
        const vector_type& translation,
        const vector_type& rotation,
        const vector_type& scale) noexcept
    {
        matrix_type s = scaling(scale);
        matrix_type rt = affine_transform(translation, rotation);
        return mul(s, rt);
    }
};
} // namespace ash::math