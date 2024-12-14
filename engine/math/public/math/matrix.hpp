#pragma once

#include "math/quaternion.hpp"

namespace violet
{
struct matrix
{
    template <typename T>
    [[nodiscard]] static inline mat4<T> mul(const mat4<T>& m1, const mat4<T>& m2) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            mat4<T> result;

            __m128 temp;

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
        }
        else
        {
            mat4<T> result(0);

            for (std::size_t i = 0; i < 4; ++i)
            {
                for (std::size_t j = 0; j < 4; ++j)
                {
                    for (std::size_t k = 0; k < 4; ++k)
                    {
                        result[i][j] += m1[i][k] * m2[k][j];
                    }
                }
            }

            return result;
        }
    }

    template <typename T>
    [[nodiscard]] static inline vec4<T> mul(const vec4<T>& v, const mat4<T>& m) noexcept
    {
        if constexpr (std::is_same_v<T, simd>)
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
        else
        {
            return {
                m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
                m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
                m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w,
                m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w};
        }
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T> mul(const mat4<T>& m, mat4<T>::value_type scale) noexcept
    {
        mat4<T> result;
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 s = _mm_set_ps1(scale);

            result[0] = _mm_mul_ps(m[0], s);
            result[1] = _mm_mul_ps(m[1], s);
            result[2] = _mm_mul_ps(m[2], s);
            result[3] = _mm_mul_ps(m[3], s);
        }
        else
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                for (std::size_t j = 0; j < 4; ++j)
                {
                    result[i][j] = m[i][j] * scale;
                }
            }
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T> transpose(const mat4<T>& m) noexcept
    {
        mat4<T> result;
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]);
            __m128 t2 = simd::shuffle<0, 1, 0, 1>(m[2], m[3]);
            __m128 t3 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]);
            __m128 t4 = simd::shuffle<2, 3, 2, 3>(m[2], m[3]);

            result[0] = simd::shuffle<0, 2, 0, 2>(t1, t2);
            result[1] = simd::shuffle<1, 3, 1, 3>(t1, t2);
            result[2] = simd::shuffle<0, 2, 0, 2>(t3, t4);
            result[3] = simd::shuffle<1, 3, 1, 3>(t3, t4);
        }
        else
        {
            for (std::size_t i = 0; i < 4; ++i)
            {
                for (std::size_t j = 0; j < 4; ++j)
                {
                    result[j][i] = m[i][j];
                }
            }
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T>::value_type determinant(const mat4<T>& m) noexcept
    {
        static_assert(!std::is_same_v<T, simd>);

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

    template <typename T>
    [[nodiscard]] static inline mat4<T> inverse(const mat4<T>& m)
    {
        mat4<T> result;
        if constexpr (std::is_same_v<T, simd>)
        {
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

            result[0] = simd::shuffle<3, 1, 3, 1>(x, y);
            result[1] = simd::shuffle<2, 0, 2, 0>(x, y);
            result[2] = simd::shuffle<3, 1, 3, 1>(z, w);
            result[3] = simd::shuffle<2, 0, 2, 0>(z, w);
        }
        else
        {
            float a2323 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
            float a1323 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
            float a1223 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
            float a0323 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
            float a0223 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
            float a0123 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
            float a2313 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
            float a1313 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
            float a1213 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
            float a2312 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
            float a1312 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
            float a1212 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
            float a0313 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
            float a0213 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
            float a0312 = m[1][0] * m[2][3] - m[1][3] * m[2][0];
            float a0212 = m[1][0] * m[2][2] - m[1][2] * m[2][0];
            float a0113 = m[1][0] * m[3][1] - m[1][1] * m[3][0];
            float a0112 = m[1][0] * m[2][1] - m[1][1] * m[2][0];

            float det = m[0][0] * (m[1][1] * a2323 - m[1][2] * a1323 + m[1][3] * a1223) -
                        m[0][1] * (m[1][0] * a2323 - m[1][2] * a0323 + m[1][3] * a0223) +
                        m[0][2] * (m[1][0] * a1323 - m[1][1] * a0323 + m[1][3] * a0123) -
                        m[0][3] * (m[1][0] * a1223 - m[1][1] * a0223 + m[1][2] * a0123);
            det = 1.0f / det;

            result[0][0] = det * (m[1][1] * a2323 - m[1][2] * a1323 + m[1][3] * a1223);
            result[0][1] = det * -(m[0][1] * a2323 - m[0][2] * a1323 + m[0][3] * a1223);
            result[0][2] = det * (m[0][1] * a2313 - m[0][2] * a1313 + m[0][3] * a1213);
            result[0][3] = det * -(m[0][1] * a2312 - m[0][2] * a1312 + m[0][3] * a1212);
            result[1][0] = det * -(m[1][0] * a2323 - m[1][2] * a0323 + m[1][3] * a0223);
            result[1][1] = det * (m[0][0] * a2323 - m[0][2] * a0323 + m[0][3] * a0223);
            result[1][2] = det * -(m[0][0] * a2313 - m[0][2] * a0313 + m[0][3] * a0213);
            result[1][3] = det * (m[0][0] * a2312 - m[0][2] * a0312 + m[0][3] * a0212);
            result[2][0] = det * (m[1][0] * a1323 - m[1][1] * a0323 + m[1][3] * a0123);
            result[2][1] = det * -(m[0][0] * a1323 - m[0][1] * a0323 + m[0][3] * a0123);
            result[2][2] = det * (m[0][0] * a1313 - m[0][1] * a0313 + m[0][3] * a0113);
            result[2][3] = det * -(m[0][0] * a1312 - m[0][1] * a0312 + m[0][3] * a0112);
            result[3][0] = det * -(m[1][0] * a1223 - m[1][1] * a0223 + m[1][2] * a0123);
            result[3][1] = det * (m[0][0] * a1223 - m[0][1] * a0223 + m[0][2] * a0123);
            result[3][2] = det * -(m[0][0] * a1213 - m[0][1] * a0213 + m[0][2] * a0113);
            result[3][3] = det * (m[0][0] * a1212 - m[0][1] * a0212 + m[0][2] * a0112);
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T> inverse_transform(const mat4<T>& m)
    {
        mat4<T> result;
        if constexpr (std::is_same_v<T, simd>)
        {
            // Transpose 3x3.
            __m128 t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]); // [m11, m12, m21, m22]
            __m128 t2 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]); // [m13, m14, m23, m24]
            result[0] = simd::shuffle<0, 2, 0, 3>(t1, m[2]);   // [m11, m21, m31, 0  ]
            result[1] = simd::shuffle<1, 3, 1, 3>(t1, m[2]);   // [m12, m22, m32, 0  ]
            result[2] = simd::shuffle<0, 2, 2, 3>(t2, m[2]);   // [m13, m23, m33, 0  ]

            t2 = _mm_mul_ps(result[0], result[0]);
            t2 = _mm_add_ps(t2, _mm_mul_ps(result[1], result[1]));
            t2 = _mm_add_ps(t2, _mm_mul_ps(result[2], result[2]));
            t2 = _mm_add_ps(_mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f), t2);
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
        }
        else
        {
            float x_scale = 1.0f / (m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]);
            float y_scale = 1.0f / (m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]);
            float z_scale = 1.0f / (m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]);

            result[0] = {m[0][0] * x_scale, m[1][0] * y_scale, m[2][0] * z_scale, 0.0f};
            result[1] = {m[0][1] * x_scale, m[1][1] * y_scale, m[2][1] * z_scale, 0.0f};
            result[2] = {m[0][2] * x_scale, m[1][2] * y_scale, m[2][2] * z_scale, 0.0f};

            result[3][0] = (-m[3][0] * m[0][0] + -m[3][1] * m[0][1] + -m[3][2] * m[0][2]) * x_scale;
            result[3][1] = (-m[3][0] * m[1][0] + -m[3][1] * m[1][1] + -m[3][2] * m[1][2]) * y_scale;
            result[3][2] = (-m[3][0] * m[2][0] + -m[3][1] * m[2][1] + -m[3][2] * m[2][2]) * z_scale;
            result[3][3] = 1.0f;
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T> inverse_transform_no_scale(const mat4<T>& m)
    {
        mat4<T> result;
        if constexpr (std::is_same_v<T, simd>)
        {
            // Transpose 3x3.
            __m128 t1 = simd::shuffle<0, 1, 0, 1>(m[0], m[1]); // [m11, m12, m21, m22]
            __m128 t2 = simd::shuffle<2, 3, 2, 3>(m[0], m[1]); // [m13, m14, m23, m24]
            result[0] = simd::shuffle<0, 2, 0, 3>(t1, m[2]);   // [m11, m21, m31, 0  ]
            result[1] = simd::shuffle<1, 3, 1, 3>(t1, m[2]);   // [m12, m22, m32, 0  ]
            result[2] = simd::shuffle<0, 2, 2, 3>(t2, m[2]);   // [m13, m23, m33, 0  ]

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
        }
        else
        {
            result[0] = {m[0][0], m[1][0], m[2][0], 0.0f};
            result[1] = {m[0][1], m[1][1], m[2][1], 0.0f};
            result[2] = {m[0][2], m[1][2], m[2][2], 0.0f};

            result[3][0] = {-m[3][0] * m[0][0] + -m[3][1] * m[0][1] + -m[3][2] * m[0][2]};
            result[3][1] = {-m[3][0] * m[1][0] + -m[3][1] * m[1][1] + -m[3][2] * m[1][2]};
            result[3][2] = {-m[3][0] * m[2][0] + -m[3][1] * m[2][1] + -m[3][2] * m[2][2]};
            result[3][3] = 1.0f;
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T> scale(const vec3<T>& s)
    {
        return {
            {s.x, 0, 0, 0},
            {0, s.y, 0, 0},
            {0, 0, s.z, 0},
            {0, 0, 0, 1},
        };
    }

    [[nodiscard]] static inline mat4f_simd scale(vec4f_simd s)
    {
        return {
            _mm_and_ps(s, simd::mask_v<1, 0, 0, 0>),
            _mm_and_ps(s, simd::mask_v<0, 1, 0, 0>),
            _mm_and_ps(s, simd::mask_v<0, 0, 1, 0>),
            _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f)};
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T> rotation_axis(const vec3<T>& axis, float radians)
    {
        auto [sin, cos] = math::sin_cos(radians);

        float x2 = axis[0] * axis[0];
        float xy = axis[0] * axis[1];
        float xz = axis[0] * axis[2];
        float y2 = axis[1] * axis[1];
        float yz = axis[1] * axis[2];
        float z2 = axis[2] * axis[2];

        mat4f result;
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

    template <typename T>
    [[nodiscard]] static inline mat4<T> rotation_quaternion(const vec4<T>& quaternion)
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

        return {
            {1.0f - yyd - zzd, xyd + zwd, xzd - ywd, 0.0f},
            {xyd - zwd, 1.0f - xxd - zzd, yzd + xwd, 0.0f},
            {xzd + ywd, yzd - xwd, 1.0f - xxd - yyd, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}};
    }

    [[nodiscard]] static inline mat4f_simd rotation_quaternion(vec4f_simd quaternion)
    {
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

        mat4f_simd result;
        result[0] = q1;

        q1 = simd::shuffle<1, 3, 2, 3>(r0, v0);
        q1 = simd::shuffle<2, 0, 3, 1>(q1);
        result[1] = q1;

        q1 = simd::shuffle<0, 1, 2, 3>(v1, r0);
        result[2] = q1;

        result[3] = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);

        return result;
    }

    template <typename T>
    [[nodiscard]] static inline mat4<T> affine_transform(
        const vec3<T>& scale,
        const vec4<T>& rotation,
        const vec3<T>& translation)
    {
        mat4<T> r = rotation_quaternion(rotation);

        return {
            {scale[0] * r[0][0], scale[0] * r[0][1], scale[0] * r[0][2], 0.0f},
            {scale[1] * r[1][0], scale[1] * r[1][1], scale[1] * r[1][2], 0.0f},
            {scale[2] * r[2][0], scale[2] * r[2][1], scale[2] * r[2][2], 0.0f},
            {translation[0], translation[1], translation[2], 1.0f}};
    }

    [[nodiscard]] static inline mat4f_simd affine_transform(
        vec4f_simd scale,
        vec4f_simd rotation,
        vec4f_simd translation)
    {
        mat4f_simd r = rotation_quaternion(rotation);
        __m128 t = _mm_and_ps(translation, simd::mask_v<1, 1, 1, 0>);

        __m128 s1 = simd::replicate<0>(scale);
        __m128 s2 = simd::replicate<1>(scale);
        __m128 s3 = simd::replicate<2>(scale);

        mat4f_simd result;
        result[0] = _mm_mul_ps(s1, r[0]);
        result[1] = _mm_mul_ps(s2, r[1]);
        result[2] = _mm_mul_ps(s3, r[2]);
        result[3] = _mm_add_ps(_mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f), t);

        return result;
    }

    template <typename T>
    static inline void decompose(
        const mat4<T>& m,
        vec3<T>& scale,
        vec4<T>& rotation,
        vec3<T>& translation)
    {
        scale[0] = vector::length(m[0]);
        scale[1] = vector::length(m[1]);
        scale[2] = vector::length(m[2]);

        mat4f r = {
            vector::mul(m[0], 1.0f / scale[0]),
            vector::mul(m[1], 1.0f / scale[1]),
            vector::mul(m[2], 1.0f / scale[2]),
            {0.0f, 0.0f, 0.0f, 1.0f}};
        rotation = quaternion::from_matrix(r);

        translation = {m[3].x, m[3].y, m[3].z};
    }

    static inline void decompose(
        const mat4f_simd& m,
        vec4f_simd& scale,
        vec4f_simd& rotation,
        vec4f_simd& translation)
    {
        __m128 s0 = vector::length_v(m[0]);
        __m128 s1 = vector::length_v(m[1]);
        __m128 s2 = vector::length_v(m[2]);

        scale = _mm_and_ps(simd::mask_v<1, 0, 0, 0>, s0);
        scale = _mm_add_ps(scale, _mm_and_ps(simd::mask_v<0, 1, 0, 0>, s1));
        scale = _mm_add_ps(scale, _mm_and_ps(simd::mask_v<0, 0, 1, 0>, s2));

        mat4f_simd r = {
            _mm_div_ps(m[0], s0),
            _mm_div_ps(m[1], s1),
            _mm_div_ps(m[2], s2),
            _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f)};
        rotation = quaternion::from_matrix(r);

        translation = m[3];
    }

    template <typename T = float>
    [[nodiscard]] static inline mat4<T> orthographic(
        mat4<T>::value_type width,
        mat4<T>::value_type height,
        mat4<T>::value_type near_z,
        mat4<T>::value_type far_z)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            float d = 1.0f / (far_z - near_z);
            __m128 t1 = _mm_setr_ps(2.0f / width, 2.0f / height, d, near_z * -d);

            mat4f_simd result;

            __m128 t2 = _mm_setzero_ps();
            result[0] = _mm_move_ss(t2, t1);
            result[1] = _mm_and_ps(t1, simd::mask_v<0, 1, 0, 0>);
            result[2] = _mm_and_ps(t1, simd::mask_v<0, 0, 1, 0>);

            // [d, near_z * -d, 0.0, 1.0]
            t1 = simd::shuffle<2, 3, 2, 3>(t1, _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f));

            // [0.0, 0.0, d, near_z * -d]
            result[3] = simd::shuffle<0, 1, 1, 3>(t2, t1);

            return result;
        }
        else
        {
            float d = 1.0f / (far_z - near_z);
            return {
                {2.0f / width, 0.0f, 0.0f, 0.0f},
                {0.0f, 2.0f / height, 0.0f, 0.0f},
                {0.0f, 0.0f, d, 0.0f},
                {0.0f, 0.0f, near_z * -d, 1.0f}};
        }
    }

    template <typename T = float>
    [[nodiscard]] static inline mat4<T> orthographic(
        mat4<T>::value_type left,
        mat4<T>::value_type right,
        mat4<T>::value_type bottom,
        mat4<T>::value_type top,
        mat4<T>::value_type near_z,
        mat4<T>::value_type far_z)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            __m128 t1 = vector::set(left, bottom, near_z, 1.0f);
            __m128 t2 = vector::set(right, top, far_z, 0.0f);
            __m128 t3 = _mm_div_ps(simd::one, _mm_sub_ps(t2, t1));

            mat4f_simd result;
            t2 = _mm_add_ps(t1, t2);
            t2 = simd::shuffle<0, 1, 2, 3>(t2, t1);
            result[3] = _mm_mul_ps(t2, _mm_sub_ps(_mm_setzero_ps(), t3));
            result[2] = _mm_and_ps(simd::mask_v<0, 0, 1, 0>, t3);

            t3 = _mm_add_ps(t3, t3);
            result[0] = _mm_and_ps(simd::mask_v<1, 0, 0, 0>, t3);
            result[1] = _mm_and_ps(simd::mask_v<0, 1, 0, 0>, t3);

            return result;
        }
        else
        {
            using value_type = mat4<T>::value_type;

            value_type w = value_type(1) / (right - left);
            value_type h = value_type(1) / (top - bottom);
            value_type d = value_type(1) / (far_z - near_z);
            return {
                {w + w, value_type(0), value_type(0), value_type(0)},
                {value_type(0), h + h, value_type(0), value_type(0)},
                {value_type(0), 0, d, value_type(0)},
                {(left + right) * -w, (bottom + top) * -h, near_z * -d, value_type(1)}};
        }
    }

    template <typename T = float>
    [[nodiscard]] static inline mat4<T> perspective(
        mat4<T>::value_type fov,
        mat4<T>::value_type aspect,
        mat4<T>::value_type near_z,
        mat4<T>::value_type far_z)
    {
        if constexpr (std::is_same_v<T, simd>)
        {
            // TODO
            mat4f result = perspective(fov, aspect, near_z, far_z);
            return math::load(result);
        }
        else
        {
            using value_type = mat4<T>::value_type;

            value_type h = value_type(1) / tanf(fov * 0.5f); // view space height
            value_type w = h / aspect;                       // view space width
            return {
                {w, value_type(0), value_type(0), value_type(0)},
                {value_type(0), h, value_type(0), value_type(0)},
                {value_type(0), value_type(0), far_z / (far_z - near_z), value_type(1)},
                {value_type(0), value_type(0), near_z * far_z / (near_z - far_z), value_type(0)}};
        }
    }

    template <typename T = float>
    [[nodiscard]] static inline mat4<T> perspective_reverse_z(
        mat4<T>::value_type fov,
        mat4<T>::value_type aspect,
        mat4<T>::value_type near_z,
        mat4<T>::value_type far_z)
    {
        return perspective<T>(fov, aspect, far_z, near_z);
    }

    [[nodiscard]] static inline mat4f_simd set(
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

    [[nodiscard]] static inline mat4f_simd set(
        vec4f_simd r0,
        vec4f_simd r1,
        vec4f_simd r2,
        vec4f_simd r3)
    {
        return {r0, r1, r2, r3};
    }

private:
    [[nodiscard]] static inline vec4f_simd mul_mat2(vec4f_simd a, vec4f_simd b)
    {
        __m128 t1 = _mm_mul_ps(a, simd::shuffle<0, 3, 0, 3>(b));
        __m128 t2 = _mm_mul_ps(simd::shuffle<1, 0, 3, 2>(a), simd::shuffle<2, 1, 2, 1>(b));
        return _mm_add_ps(t1, t2);
    }

    [[nodiscard]] static inline vec4f_simd mul_adj_mat2(vec4f_simd a, vec4f_simd b)
    {
        __m128 t1 = _mm_mul_ps(simd::shuffle<3, 3, 0, 0>(a), b);
        __m128 t2 = _mm_mul_ps(simd::shuffle<1, 1, 2, 2>(a), simd::shuffle<2, 3, 0, 1>(b));
        return _mm_sub_ps(t1, t2);
    }

    [[nodiscard]] static inline vec4f_simd mul_mat2_adj(vec4f_simd a, vec4f_simd b)
    {
        __m128 t1 = _mm_mul_ps(a, simd::shuffle<3, 0, 3, 0>(b));
        __m128 t2 = _mm_mul_ps(simd::shuffle<1, 0, 3, 2>(a), simd::shuffle<2, 1, 2, 1>(b));
        return _mm_sub_ps(t1, t2);
    }
};
} // namespace violet