#pragma once

#include "math/quaternion.hpp"
#include "math/utility.hpp"
#include "math/vector.hpp"

namespace violet
{
class matrix
{
public:
    [[nodiscard]] static inline matrix4 set(
        float m00,
        float m01,
        float m02,
        float m03,
        float m10,
        float m11,
        float m12,
        float m13,
        float m20,
        float m21,
        float m22,
        float m23,
        float m30,
        float m31,
        float m32,
        float m33)
    {
        return {
            vector::set(m00, m01, m02, m03),
            vector::set(m10, m11, m12, m13),
            vector::set(m20, m21, m22, m23),
            vector::set(m30, m31, m32, m33)};
    }

    [[nodiscard]] static inline matrix4 set(vector4 r0, vector4 r1, vector4 r2, vector4 r3)
    {
        return {r0, r1, r2, r3};
    }

    template <typename T>
    [[nodiscard]] static inline T identity();

    template <>
    [[nodiscard]] static inline float4x4 identity<float4x4>()
    {
        return {
            float4{1.0f, 0.0f, 0.0f, 0.0f},
            float4{0.0f, 1.0f, 0.0f, 0.0f},
            float4{0.0f, 0.0f, 1.0f, 0.0f},
            float4{0.0f, 0.0f, 0.0f, 1.0f}
        };
    }

#ifdef VIOLET_USE_SIMD
    template <>
    [[nodiscard]] static inline matrix4 identity<matrix4>()
    {
        return {
            math::IDENTITY_ROW_0,
            math::IDENTITY_ROW_1,
            math::IDENTITY_ROW_2,
            math::IDENTITY_ROW_3};
    }
#endif

    [[nodiscard]] static inline matrix4 mul(const matrix4& m1, const matrix4& m2)
    {
#ifdef VIOLET_USE_SIMD
        __m128 temp;
        matrix4 result;

        // row 1
        __m128 row = m1[0];
        temp = _mm_mul_ps(simd::replicate<0>(row), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(row), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(row), m2[2]), temp);
        result[0] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(row), m2[3]), temp);

        // row 2
        row = m1[1];
        temp = _mm_mul_ps(simd::replicate<0>(row), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(row), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(row), m2[2]), temp);
        result[1] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(row), m2[3]), temp);

        // row 3
        row = m1[2];
        temp = _mm_mul_ps(simd::replicate<0>(row), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(row), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(row), m2[2]), temp);
        result[2] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(row), m2[3]), temp);

        // row 4
        row = m1[3];
        temp = _mm_mul_ps(simd::replicate<0>(row), m2[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(row), m2[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(row), m2[2]), temp);
        result[3] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(row), m2[3]), temp);

        return result;
#else
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
#endif
    }

    [[nodiscard]] static inline vector4f mul(vector4f v, const matrix4& m)
    {
#ifdef VIOLET_USE_SIMD
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
#else
        return {
            m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3],
            m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3],
            m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3],
            m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3]};
#endif
    }

    [[nodiscard]] static inline matrix4 mul(const matrix4& m, float scale)
    {
#ifdef VIOLET_USE_SIMD
        matrix4 result;
        __m128 s = _mm_set_ps1(scale);

        result[0] = _mm_mul_ps(m[0], s);
        result[1] = _mm_mul_ps(m[1], s);
        result[2] = _mm_mul_ps(m[2], s);
        result[3] = _mm_mul_ps(m[3], s);

        return result;
#else
        float4x4 result = {};
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
                result[i][j] = m[i][j] * scale;
        }
        return result;
#endif
    }

    [[nodiscard]] static inline matrix4 transpose(const matrix4& m)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]);
        __m128 t2 = simd::shuffle<0, 1, 0, 1>(m[2], m[3]);
        __m128 t3 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]);
        __m128 t4 = simd::shuffle<2, 3, 2, 3>(m[2], m[3]);

        matrix4 result;
        result[0] = simd::shuffle<0, 2, 0, 2>(t1, t2);
        result[1] = simd::shuffle<1, 3, 1, 3>(t1, t2);
        result[2] = simd::shuffle<0, 2, 0, 2>(t3, t4);
        result[3] = simd::shuffle<1, 3, 1, 3>(t3, t4);

        return result;
#else
        float4x4 result = {};
        for (std::size_t i = 0; i < 4; ++i)
        {
            for (std::size_t j = 0; j < 4; ++j)
                result[j][i] = m[i][j];
        }
        return result;
#endif
    }

    [[nodiscard]] static inline float determinant(const matrix4& m)
    {
#ifdef VIOLET_USE_SIMD
#else
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
#endif
    }

    [[nodiscard]] static inline matrix4 inverse(const matrix4& m)
    {
#ifdef VIOLET_USE_SIMD
        // https://lxjk.github.io/2017/09/03/Fast-4x4-Matrix-Inverse-with-SSE-SIMD-Explained.html

        __m128 sub_a = simd::shuffle<0, 1, 0, 1>(m[0], m[1]);
        __m128 sub_b = simd::shuffle<2, 3, 2, 3>(m[0], m[1]);
        __m128 sub_c = simd::shuffle<0, 1, 0, 1>(m[2], m[3]);
        __m128 sub_d = simd::shuffle<2, 3, 2, 3>(m[2], m[3]);

        __m128 sub_det = _mm_sub_ps(
            _mm_mul_ps(
                simd::shuffle<0, 2, 0, 2>(m[0], m[2]),
                simd::shuffle<1, 3, 1, 3>(m[1], m[3])),
            _mm_mul_ps(
                simd::shuffle<1, 3, 1, 3>(m[0], m[2]),
                simd::shuffle<0, 2, 0, 2>(m[1], m[3])));

        __m128 det_a = simd::replicate<0>(sub_det);
        __m128 det_b = simd::replicate<1>(sub_det);
        __m128 det_c = simd::replicate<2>(sub_det);
        __m128 det_d = simd::replicate<3>(sub_det);

        __m128 dc = mul_adj_mat2(sub_d, sub_c);
        __m128 ab = mul_adj_mat2(sub_a, sub_b);
        __m128 x = _mm_sub_ps(_mm_mul_ps(det_d, sub_a), mul_mat2(sub_b, dc));
        __m128 w = _mm_sub_ps(_mm_mul_ps(det_a, sub_d), mul_mat2(sub_c, ab));

        __m128 det_m = _mm_mul_ps(det_a, det_d);

        __m128 y = _mm_sub_ps(_mm_mul_ps(det_b, sub_c), mul_mat2_adj(sub_d, ab));
        __m128 z = _mm_sub_ps(_mm_mul_ps(det_c, sub_b), mul_mat2_adj(sub_a, dc));

        det_m = _mm_add_ps(det_m, _mm_mul_ps(det_b, det_c));

        __m128 tr = _mm_mul_ps(ab, simd::shuffle<0, 2, 1, 3>(dc));
        tr = _mm_hadd_ps(tr, tr);
        tr = _mm_hadd_ps(tr, tr);
        det_m = _mm_sub_ps(det_m, tr);

        __m128 rdet_m = _mm_div_ps(_mm_setr_ps(1.f, -1.f, -1.f, 1.f), det_m);

        x = _mm_mul_ps(x, rdet_m);
        y = _mm_mul_ps(y, rdet_m);
        z = _mm_mul_ps(z, rdet_m);
        w = _mm_mul_ps(w, rdet_m);

        matrix4 result;
        result[0] = simd::shuffle<3, 1, 3, 1>(x, y);
        result[1] = simd::shuffle<2, 0, 2, 0>(x, y);
        result[2] = simd::shuffle<3, 1, 3, 1>(z, w);
        result[3] = simd::shuffle<2, 0, 2, 0>(z, w);

        return result;
#else
        float A2323 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
        float A1323 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
        float A1223 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
        float A0323 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
        float A0223 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
        float A0123 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
        float A2313 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
        float A1313 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
        float A1213 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
        float A2312 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
        float A1312 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
        float A1212 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
        float A0313 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
        float A0213 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
        float A0312 = m[1][0] * m[2][3] - m[1][3] * m[2][0];
        float A0212 = m[1][0] * m[2][2] - m[1][2] * m[2][0];
        float A0113 = m[1][0] * m[3][1] - m[1][1] * m[3][0];
        float A0112 = m[1][0] * m[2][1] - m[1][1] * m[2][0];

        float det = m[0][0] * (m[1][1] * A2323 - m[1][2] * A1323 + m[1][3] * A1223) -
                    m[0][1] * (m[1][0] * A2323 - m[1][2] * A0323 + m[1][3] * A0223) +
                    m[0][2] * (m[1][0] * A1323 - m[1][1] * A0323 + m[1][3] * A0123) -
                    m[0][3] * (m[1][0] * A1223 - m[1][1] * A0223 + m[1][2] * A0123);
        det = 1.0f / det;

        float4x4 result;
        result[0][0] = det * (m[1][1] * A2323 - m[1][2] * A1323 + m[1][3] * A1223);
        result[0][1] = det * -(m[0][1] * A2323 - m[0][2] * A1323 + m[0][3] * A1223);
        result[0][2] = det * (m[0][1] * A2313 - m[0][2] * A1313 + m[0][3] * A1213);
        result[0][3] = det * -(m[0][1] * A2312 - m[0][2] * A1312 + m[0][3] * A1212);
        result[1][0] = det * -(m[1][0] * A2323 - m[1][2] * A0323 + m[1][3] * A0223);
        result[1][1] = det * (m[0][0] * A2323 - m[0][2] * A0323 + m[0][3] * A0223);
        result[1][2] = det * -(m[0][0] * A2313 - m[0][2] * A0313 + m[0][3] * A0213);
        result[1][3] = det * (m[0][0] * A2312 - m[0][2] * A0312 + m[0][3] * A0212);
        result[2][0] = det * (m[1][0] * A1323 - m[1][1] * A0323 + m[1][3] * A0123);
        result[2][1] = det * -(m[0][0] * A1323 - m[0][1] * A0323 + m[0][3] * A0123);
        result[2][2] = det * (m[0][0] * A1313 - m[0][1] * A0313 + m[0][3] * A0113);
        result[2][3] = det * -(m[0][0] * A1312 - m[0][1] * A0312 + m[0][3] * A0112);
        result[3][0] = det * -(m[1][0] * A1223 - m[1][1] * A0223 + m[1][2] * A0123);
        result[3][1] = det * (m[0][0] * A1223 - m[0][1] * A0223 + m[0][2] * A0123);
        result[3][2] = det * -(m[0][0] * A1213 - m[0][1] * A0213 + m[0][2] * A0113);
        result[3][3] = det * (m[0][0] * A1212 - m[0][1] * A0212 + m[0][2] * A0112);

        return result;
#endif
    }

    [[nodiscard]] static inline matrix4 inverse_transform(const matrix4& m)
    {
#ifdef VIOLET_USE_SIMD
        matrix4 result;

        // Transpose 3x3.
        vector4f t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]); // [m11, m12, m21, m22]
        vector4f t2 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]); // [m13, m14, m23, m24]
        result[0] = simd::shuffle<0, 2, 0, 3>(t1, m[2]);     // [m11, m21, m31, 0  ]
        result[1] = simd::shuffle<1, 3, 1, 3>(t1, m[2]);     // [m12, m22, m32, 0  ]
        result[2] = simd::shuffle<0, 2, 2, 3>(t2, m[2]);     // [m13, m23, m33, 0  ]

        t2 = _mm_mul_ps(result[0], result[0]);
        t2 = _mm_add_ps(t2, _mm_mul_ps(result[1], result[1]));
        t2 = _mm_add_ps(t2, _mm_mul_ps(result[2], result[2]));
        t2 = _mm_add_ps(math::IDENTITY_ROW_3, t2);
        t2 = _mm_div_ps(simd::one, t2);

        result[0] = _mm_mul_ps(result[0], t2);
        result[1] = _mm_mul_ps(result[1], t2);
        result[2] = _mm_mul_ps(result[2], t2);

        // Last line.
        t1 = simd::replicate<0>(m[3]);         // [m41, m41, m41, m41]
        result[3] = _mm_mul_ps(t1, result[0]); // [m41 * m11, m41 * m21, m41 * m31, 0]
        t1 = simd::replicate<1>(m[3]);         // [m42, m42, m42, m42]
        t1 = _mm_mul_ps(t1, result[1]);        // [m42 * m12, m42 * m22, m42 * m32, 0]
        result[3] = _mm_add_ps(result[3], t1); // [m41 * m11 + m42 * m12,
                                               //  m41 * m21 + m42 * m22,
                                               //  m41 * m31 + m42 * m32, 0]
        t1 = simd::replicate<2>(m[3]);         // [m43, m43, m43, m43]
        t1 = _mm_mul_ps(t1, result[2]);        // [m43 * m13, m43 * m23, m43 * m33, 0]
        result[3] = _mm_add_ps(result[3], t1); // [m41 * m11 + m42 * m12 + m43 * m13,
                                               //  m41 * m21 + m42 * m22 + m43 * m23,
                                               //  m41 * m31 + m42 * m32 + m43 * m33, 0]

        t1 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
        result[3] = _mm_sub_ps(t1, result[3]); // [-m41 * m11 + -m42 * m12 + -m43 * m13,
                                               //  -m41 * m21 + -m42 * m22 + -m43 * m23,
                                               //  -m41 * m31 + -m42 * m32 + -m43 * m33, 1]

        return result;
#else
        float x_scale = 1.0f / (m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
        float y_scale = 1.0f / (m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
        float z_scale = 1.0f / (m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

        float4x4 result;
        result[0] = {m[0][0] * x_scale, m[1][0] * y_scale, m[2][0] * z_scale, 0.0f};
        result[1] = {m[0][1] * x_scale, m[1][1] * y_scale, m[2][1] * z_scale, 0.0f};
        result[2] = {m[0][2] * x_scale, m[1][2] * y_scale, m[2][2] * z_scale, 0.0f};

        result[3][0] = (-m[3][0] * m[0][0] + -m[3][1] * m[0][1] + -m[3][2] * m[0][2]) * x_scale;
        result[3][1] = (-m[3][0] * m[1][0] + -m[3][1] * m[1][1] + -m[3][2] * m[1][2]) * y_scale;
        result[3][2] = (-m[3][0] * m[2][0] + -m[3][1] * m[2][1] + -m[3][2] * m[2][2]) * z_scale;
        result[3][3] = 1.0f;

        return result;
#endif
    }

    [[nodiscard]] static inline matrix4 inverse_transform_no_scale(const matrix4& m)
    {
#ifdef VIOLET_USE_SIMD
        matrix4 result;

        // Transpose 3x3.
        vector4f t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]); // [m11, m12, m21, m22]
        vector4f t2 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]); // [m13, m14, m23, m24]
        result[0] = simd::shuffle<0, 2, 0, 3>(t1, m[2]);     // [m11, m21, m31, 0  ]
        result[1] = simd::shuffle<1, 3, 1, 3>(t1, m[2]);     // [m12, m22, m32, 0  ]
        result[2] = simd::shuffle<0, 2, 2, 3>(t2, m[2]);     // [m13, m23, m33, 0  ]

        // Last line.
        t1 = simd::replicate<0>(m[3]);         // [m41, m41, m41, m41]
        result[3] = _mm_mul_ps(t1, result[0]); // [m41 * m11, m41 * m21, m41 * m31, 0]
        t1 = simd::replicate<1>(m[3]);         // [m42, m42, m42, m42]
        t1 = _mm_mul_ps(t1, result[1]);        // [m42 * m12, m42 * m22, m42 * m32, 0]
        result[3] = _mm_add_ps(result[3], t1); // [m41 * m11 + m42 * m12,
                                               //  m41 * m21 + m42 * m22,
                                               //  m41 * m31 + m42 * m32, 0]
        t1 = simd::replicate<2>(m[3]);         // [m43, m43, m43, m43]
        t1 = _mm_mul_ps(t1, result[2]);        // [m43 * m13, m43 * m23, m43 * m33, 0]
        result[3] = _mm_add_ps(result[3], t1); // [m41 * m11 + m42 * m12 + m43 * m13,
                                               //  m41 * m21 + m42 * m22 + m43 * m23,
                                               //  m41 * m31 + m42 * m32 + m43 * m33, 0]
        t1 = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
        result[3] = _mm_sub_ps(t1, result[3]); // [-m41 * m11 + -m42 * m12 + -m43 * m13,
                                               //  -m41 * m21 + -m42 * m22 + -m43 * m23,
                                               //  -m41 * m31 + -m42 * m32 + -m43 * m33, 1]

        return result;
#else
        float4x4 result;
        result[0] = {m[0][0], m[1][0], m[2][0], 0.0f};
        result[1] = {m[0][1], m[1][1], m[2][1], 0.0f};
        result[2] = {m[0][2], m[1][2], m[2][2], 0.0f};

        result[3][0] = {-m[3][0] * m[0][0] + -m[3][1] * m[0][1] + -m[3][2] * m[0][2]};
        result[3][1] = {-m[3][0] * m[1][0] + -m[3][1] * m[1][1] + -m[3][2] * m[1][2]};
        result[3][2] = {-m[3][0] * m[2][0] + -m[3][1] * m[2][1] + -m[3][2] * m[2][2]};
        result[3][3] = 1.0f;

        return result;
#endif
    }

    [[nodiscard]] static inline matrix4 scale(float x, float y, float z)
    {
#ifdef VIOLET_USE_SIMD
        return {
            _mm_setr_ps(x, 0.0f, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, y, 0.0f, 0.0f),
            _mm_setr_ps(0.0f, 0.0f, z, 0.0f),
            math::IDENTITY_ROW_3};
#else
        return {
            float4{x,    0.0f, 0.0f, 0.0f},
            float4{0.0f, y,    0.0f, 0.0f},
            float4{0.0f, 0.0f, z,    0.0f},
            float4{0.0f, 0.0f, 0.0f, 1.0f},
        };
#endif
    }

    [[nodiscard]] static inline matrix4 scale(vector4f v)
    {
#ifdef VIOLET_USE_SIMD
        return matrix4{
            _mm_and_ps(v, simd::mask_v<1, 0, 0, 0>),
            _mm_and_ps(v, simd::mask_v<0, 1, 0, 0>),
            _mm_and_ps(v, simd::mask_v<0, 0, 1, 0>),
            math::IDENTITY_ROW_3};
#else
        return scale(v[0], v[1], v[2]);
#endif
    }

    [[nodiscard]] static inline matrix4 rotation_axis(vector4f axis, float radians)
    {
#ifdef VIOLET_USE_SIMD
#else
        auto [sin, cos] = sin_cos(radians);

        float x2 = axis[0] * axis[0];
        float xy = axis[0] * axis[1];
        float xz = axis[0] * axis[2];
        float y2 = axis[1] * axis[1];
        float yz = axis[1] * axis[2];
        float z2 = axis[2] * axis[2];

        float4x4 result = identity<float4x4>();
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
#endif
    }

    [[nodiscard]] static inline matrix4 rotation_x_axis(float radians)
    {
#ifdef VIOLET_USE_SIMD
#else
        auto [sin, cos] = sin_cos(radians);

        float4x4 result = identity<float4x4>();
        result[1][1] = cos;
        result[1][2] = sin;
        result[2][1] = -sin;
        result[2][2] = cos;

        return result;
#endif
    }

    [[nodiscard]] static inline matrix4 rotation_y_axis(float radians)
    {
#ifdef VIOLET_USE_SIMD
#else
        auto [sin, cos] = sin_cos(radians);

        float4x4 result = identity<float4x4>();
        result[0][0] = cos;
        result[0][2] = -sin;
        result[2][0] = sin;
        result[2][2] = cos;

        return result;
#endif
    }

    [[nodiscard]] static inline matrix4 rotation_z_axis(float radians)
    {
#ifdef VIOLET_USE_SIMD
#else
        auto [sin, cos] = sin_cos(radians);

        float4x4 result = identity<float4x4>();
        result[0][0] = cos;
        result[0][1] = sin;
        result[1][0] = -sin;
        result[1][1] = cos;

        return result;
#endif
    }

    [[nodiscard]] static inline matrix4 rotation_quaternion(vector4f quaternion)
    {
#ifdef VIOLET_USE_SIMD
        const __m128 c = vector::set(1.0f, 1.0f, 1.0f, 0.0f);

        __m128 q0 = _mm_add_ps(quaternion, quaternion);
        __m128 q1 = _mm_mul_ps(quaternion, q0);

        __m128 v0 = simd::shuffle<1, 0, 0, 3>(q1);
        v0 = _mm_and_ps(v0, simd::mask_v<1, 1, 1, 0>);
        __m128 v1 = simd::shuffle<2, 2, 1, 3>(q1);
        v1 = _mm_and_ps(v1, simd::mask_v<1, 1, 1, 0>);
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

        matrix4 result = {q1, math::IDENTITY_ROW_1, math::IDENTITY_ROW_2, math::IDENTITY_ROW_3};

        q1 = simd::shuffle<1, 3, 2, 3>(r0, v0);
        q1 = simd::shuffle<2, 0, 3, 1>(q1);
        result[1] = q1;

        q1 = simd::shuffle<0, 1, 2, 3>(v1, r0);
        result[2] = q1;

        return result;
#else
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

        return {
            float4{1.0f - yyd - zzd, xyd + zwd,        xzd - ywd,        0.0f},
            float4{xyd - zwd,        1.0f - xxd - zzd, yzd + xwd,        0.0f},
            float4{xzd + ywd,        yzd - xwd,        1.0f - xxd - yyd, 0.0f},
            float4{0.0f,             0.0f,             0.0f,             1.0f}
        };
#endif
    }

    [[nodiscard]] static inline matrix4 affine_transform(
        vector4f scale,
        vector4f rotation,
        vector4f translation)
    {
#ifdef VIOLET_USE_SIMD
        matrix4 r = rotation_quaternion(rotation);
        __m128 t = _mm_and_ps(translation, simd::mask_v<1, 1, 1, 0>);

        __m128 s1 = simd::replicate<0>(scale);
        __m128 s2 = simd::replicate<1>(scale);
        __m128 s3 = simd::replicate<2>(scale);

        matrix4 result;
        result[0] = _mm_mul_ps(s1, r[0]);
        result[1] = _mm_mul_ps(s2, r[1]);
        result[2] = _mm_mul_ps(s3, r[2]);
        result[3] = _mm_add_ps(math::IDENTITY_ROW_3, t);

        return result;
#else
        float4x4 r = rotation_quaternion(rotation);

        return {
            float4{scale[0] * r[0][0], scale[0] * r[0][1], scale[0] * r[0][2], 0.0f},
            float4{scale[1] * r[1][0], scale[1] * r[1][1], scale[1] * r[1][2], 0.0f},
            float4{scale[2] * r[2][0], scale[2] * r[2][1], scale[2] * r[2][2], 0.0f},
            float4{translation[0],     translation[1],     translation[2],     1.0f}
        };
#endif
    }

    [[nodiscard]] static inline void decompose(
        matrix4 m,
        vector4f& scale,
        vector4f& rotation,
        vector4f& translation)
    {
#ifdef VIOLET_USE_SIMD
        __m128 s0 = vector::length_v(m[0]);
        __m128 s1 = vector::length_v(m[1]);
        __m128 s2 = vector::length_v(m[2]);

        scale = _mm_and_ps(simd::mask_v<1, 0, 0, 0>, s0);
        scale = _mm_add_ps(scale, _mm_and_ps(simd::mask_v<0, 1, 0, 0>, s1));
        scale = _mm_add_ps(scale, _mm_and_ps(simd::mask_v<0, 0, 1, 0>, s2));

        matrix4 r = {
            _mm_div_ps(m[0], s0),
            _mm_div_ps(m[1], s1),
            _mm_div_ps(m[2], s2),
            math::IDENTITY_ROW_3};
        rotation = quaternion::from_matrix(r);

        translation = m[3];
#else
        scale[0] = vector::length(m[0]);
        scale[1] = vector::length(m[1]);
        scale[2] = vector::length(m[2]);
        scale[3] = 0.0f;

        float4x4 r = {
            vector::mul(m[0], 1.0f / scale[0]),
            vector::mul(m[1], 1.0f / scale[1]),
            vector::mul(m[2], 1.0f / scale[2]),
            {0.0f, 0.0f, 0.0f, 1.0f}
        };
        rotation = quaternion::from_matrix(r);

        translation = m[3];
#endif
    }

    [[nodiscard]] static inline matrix4 orthographic(
        float width,
        float height,
        float near_z,
        float far_z)
    {
#ifdef VIOLET_USE_SIMD
        float d = 1.0f / (far_z - near_z);
        __m128 t1 = _mm_setr_ps(2.0f / width, 2.0f / height, d, near_z * -d);

        matrix4 result;

        __m128 t2 = _mm_setzero_ps();
        result[0] = _mm_move_ss(t2, t1);
        result[1] = _mm_and_ps(t1, simd::mask_v<0, 1, 0, 0>);
        result[2] = _mm_and_ps(t1, simd::mask_v<0, 0, 1, 0>);

        // [d, near_z * -d, 0.0, 1.0]
        t1 = simd::shuffle<2, 3, 2, 3>(t1, math::IDENTITY_ROW_3);

        // [0.0, 0.0, d, near_z * -d]
        result[3] = simd::shuffle<0, 1, 1, 3>(t2, t1);

        return result;
#else
        float d = 1.0f / (far_z - near_z);
        return {
            float4{2.0f / width, 0.0f,          0.0f,        0.0f},
            float4{0.0f,         2.0f / height, 0.0f,        0.0f},
            float4{0.0f,         0.0f,          d,           0.0f},
            float4{0.0f,         0.0f,          near_z * -d, 1.0f}
        };
#endif
    }

    [[nodiscard]] static inline matrix4 orthographic(
        float left,
        float right,
        float bottom,
        float top,
        float near_z,
        float far_z)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = vector::set(left, bottom, near_z, 1.0f);
        __m128 t2 = vector::set(right, top, far_z, 0.0f);
        __m128 t3 = _mm_div_ps(simd::one, _mm_sub_ps(t2, t1));

        matrix4 result;
        t2 = _mm_add_ps(t1, t2);
        t2 = simd::shuffle<0, 1, 2, 3>(t2, t1);
        result[3] = _mm_mul_ps(t2, _mm_sub_ps(_mm_setzero_ps(), t3));
        result[2] = _mm_and_ps(simd::mask_v<0, 0, 1, 0>, t3);

        t3 = _mm_add_ps(t3, t3);
        result[0] = _mm_and_ps(simd::mask_v<1, 0, 0, 0>, t3);
        result[1] = _mm_and_ps(simd::mask_v<0, 1, 0, 0>, t3);

        return result;
#else
        float w = 1.0f / (right - left);
        float h = 1.0f / (top - bottom);
        float d = 1.0f / (far_z - near_z);
        return {
            float4{w + w,               0.0f,                0.0f,        0.0f},
            float4{0.0f,                h + h,               0.0f,        0.0f},
            float4{0.0f,                0,                   d,           0.0f},
            float4{(left + right) * -w, (bottom + top) * -h, near_z * -d, 1.0f}
        };
#endif
    }

    [[nodiscard]] static inline matrix4 perspective(float fov, float aspect, float zn, float zf)
    {
#ifdef VIOLET_USE_SIMD
        // TODO
        float h = 1.0f / tanf(fov * 0.5f); // view space height
        float w = h / aspect;              // view space width
        return {
            vector::set(w, 0.0f, 0.0f, 0.0f),
            vector::set(0.0f, h, 0.0f, 0.0f),
            vector::set(0.0f, 0.0f, zf / (zf - zn), 1.0f),
            vector::set(0.0f, 0.0f, zn * zf / (zn - zf), 0.0f)};
#else
        float h = 1.0f / tanf(fov * 0.5f); // view space height
        float w = h / aspect;              // view space width
        return {
            float4{w,    0.0f, 0.0f,                0.0f},
            float4{0.0f, h,    0.0f,                0.0f},
            float4{0.0f, 0.0f, zf / (zf - zn),      1.0f},
            float4{0.0f, 0.0f, zn * zf / (zn - zf), 0.0f}
        };
#endif
    }

private:
    [[nodiscard]] static inline vector4f mul_mat2(vector4f a, vector4f b)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_mul_ps(a, simd::shuffle<0, 3, 0, 3>(b));
        __m128 t2 = _mm_mul_ps(simd::shuffle<1, 0, 3, 2>(a), simd::shuffle<2, 1, 2, 1>(b));
        return _mm_add_ps(t1, t2);
#else
#endif
    }

    [[nodiscard]] static inline vector4f mul_adj_mat2(vector4f a, vector4f b)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_mul_ps(simd::shuffle<3, 3, 0, 0>(a), b);
        __m128 t2 = _mm_mul_ps(simd::shuffle<1, 1, 2, 2>(a), simd::shuffle<2, 3, 0, 1>(b));
        return _mm_sub_ps(t1, t2);
#else
#endif
    }

    [[nodiscard]] static inline vector4f mul_mat2_adj(vector4f a, vector4f b)
    {
#ifdef VIOLET_USE_SIMD
        __m128 t1 = _mm_mul_ps(a, simd::shuffle<3, 0, 3, 0>(b));
        __m128 t2 = _mm_mul_ps(simd::shuffle<1, 0, 3, 2>(a), simd::shuffle<2, 1, 2, 1>(b));
        return _mm_sub_ps(t1, t2);
#else
#endif
    }
};
} // namespace violet