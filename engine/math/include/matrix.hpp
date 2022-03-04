#pragma once

#include "simd.hpp"
#include "type.hpp"

namespace ash::math
{
struct matrix
{
public:
    template <typename M1, typename M2>
    struct mul_result
    {
        using value_type = packed_trait<M1>::value_type;
        using type = packed_2d<value_type, packed_trait<M1>::row_size, packed_trait<M2>::col_size>;
    };

    template <>
    struct mul_result<float4x4_simd, float4x4_simd>
    {
        using value_type = packed_trait<float4x4_simd>::value_type;
        using type = float4x4_simd;
    };

    template <typename M1, typename M2>
    using mul_result_t = mul_result<M1, M2>::type;

    template <typename M1, typename M2>
    inline static mul_result_t<M1, M2> mul(const M1& m1, const M2& m2)
    {
        mul_result_t<M1, M2> result = {};

        for (std::size_t i = 0; i < packed_trait<M1>::row_size; ++i)
        {
            for (std::size_t j = 0; j < packed_trait<M2>::col_size; ++j)
            {
                for (std::size_t k = 0; k < packed_trait<M1>::col_size; ++k)
                    result[i][j] += m1[i][k] * m2[k][j];
            }
        }

        return result;
    }

    template <>
    inline static float4x4_simd mul(const float4x4_simd& m1, const float4x4_simd& m2)
    {
        __m128 temp;
        float4x4_simd result;

        // row 1
        temp = _mm_mul_ps(simd::replicate<0>(m1.row[0]), m2.row[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1.row[0]), m2.row[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1.row[0]), m2.row[2]), temp);
        result.row[0] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1.row[0]), m2.row[3]), temp);

        // row 2
        temp = _mm_mul_ps(simd::replicate<0>(m1.row[1]), m2.row[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1.row[1]), m2.row[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1.row[1]), m2.row[2]), temp);
        result.row[1] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1.row[1]), m2.row[3]), temp);

        // row 3
        temp = _mm_mul_ps(simd::replicate<0>(m1.row[2]), m2.row[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1.row[2]), m2.row[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1.row[2]), m2.row[2]), temp);
        result.row[2] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1.row[2]), m2.row[3]), temp);

        // row 4
        temp = _mm_mul_ps(simd::replicate<0>(m1.row[3]), m2.row[0]);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<1>(m1.row[3]), m2.row[1]), temp);
        temp = _mm_add_ps(_mm_mul_ps(simd::replicate<2>(m1.row[3]), m2.row[2]), temp);
        result.row[3] = _mm_add_ps(_mm_mul_ps(simd::replicate<3>(m1.row[3]), m2.row[3]), temp);

        return result;
    }

    template <typename M, typename S>
    inline static M scale(const M& m, S scale)
    {
        M result = {};
        for (std::size_t i = 0; i < packed_trait<M>::row_size; ++i)
        {
            for (std::size_t j = 0; j < packed_trait<M>::col_size; ++j)
                result[i][j] = m[i][j] * scale;
        }
        return result;
    }

    template <>
    inline static float4x4_simd scale(const float4x4_simd& m, float scale)
    {
        float4x4_simd result;
        __m128 s = _mm_set_ps1(scale);

        result.row[0] = _mm_mul_ps(m.row[0], s);
        result.row[1] = _mm_mul_ps(m.row[1], s);
        result.row[2] = _mm_mul_ps(m.row[2], s);
        result.row[3] = _mm_mul_ps(m.row[3], s);

        return result;
    }

    template <typename M>
    struct transpose_result
    {
        using value_type = packed_trait<M>::value_type;
        using type = packed_2d<value_type, packed_trait<M>::col_size, packed_trait<M>::row_size>;
    };

    template <>
    struct transpose_result<float4x4_simd>
    {
        using value_type = packed_trait<float4x4_simd>::value_type;
        using type = float4x4_simd;
    };

    template <typename M>
    using transpose_result_t = transpose_result<M>::type;

    template <typename M>
    inline static transpose_result_t<M> transpose(const M& m)
    {
        transpose_result_t<M> result = {};

        for (std::size_t i = 0; i < packed_trait<M>::row_size; ++i)
        {
            for (std::size_t j = 0; j < packed_trait<M>::col_size; ++j)
                result[j][i] = m[i][j];
        }

        return result;
    }

    template <>
    inline static float4x4_simd transpose(const float4x4_simd& m)
    {
        __m128 t1 = simd::shuffle<0, 1, 0, 1>(m.row[0], m.row[1]);
        __m128 t2 = simd::shuffle<0, 1, 0, 1>(m.row[2], m.row[3]);
        __m128 t3 = simd::shuffle<2, 3, 2, 3>(m.row[0], m.row[1]);
        __m128 t4 = simd::shuffle<2, 3, 2, 3>(m.row[2], m.row[3]);

        float4x4_simd result;
        result.row[0] = simd::shuffle<0, 2, 0, 2>(t1, t2);
        result.row[1] = simd::shuffle<1, 3, 1, 3>(t1, t2);
        result.row[2] = simd::shuffle<0, 2, 0, 2>(t3, t4);
        result.row[3] = simd::shuffle<1, 3, 1, 3>(t3, t4);

        return result;
    }

    template <square_matrix M>
    inline static packed_trait<M>::value_type determinant(const M& m)
    {
        if constexpr (packed_trait<M>::row_size == 2)
        {
            return m[0][0] * m[1][1] - m[0][1] * m[1][0];
        }
        else if constexpr (packed_trait<M>::row_size == 3)
        {
            return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) +
                   m[0][1] * (m[1][2] * m[2][0] - m[1][0] * m[2][2]) +
                   m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
        }
        else if constexpr (packed_trait<M>::row_size == 4)
        {
            // TODO: Calculate 4x4 matrix determinant.
        }
    }

    template <>
    inline static float4x4_simd determinant(const float4x4_simd& m)
    {
        // TODO: Calculate simd version 4x4 matrix determinant.
        return {};
    }

    template <square_matrix M>
    inline static M inverse(const M& m)
    {
        if constexpr (packed_trait<M>::row_size == 4)
        {
            using value_type = packed_trait<M>::value_type;
            M result = {};

            value_type det =
                (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
                 m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                 m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]));

            value_type detInv = value_type(1.0) / det;

            result[0][0] = detInv * (m[1][1] * m[2][2] - m[1][2] * m[2][1]);
            result[0][1] = -detInv * (m[0][1] * m[2][2] - m[0][2] * m[2][1]);
            result[0][2] = detInv * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
            result[0][3] = 0.0;

            result[1][0] = -detInv * (m[1][0] * m[2][2] - m[1][2] * m[2][0]);
            result[1][1] = detInv * (m[0][0] * m[2][2] - m[0][2] * m[2][0]);
            result[1][2] = -detInv * (m[0][0] * m[1][2] - m[0][2] * m[1][0]);
            result[1][3] = 0.0;

            result[2][0] = detInv * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
            result[2][1] = -detInv * (m[0][0] * m[2][1] - m[0][1] * m[2][0]);
            result[2][2] = detInv * (m[0][0] * m[1][1] - m[0][1] * m[1][0]);
            result[2][3] = 0.0;

            result[3][0] =
                -(m[3][0] * result[0][0] + m[3][1] * result[1][0] + m[3][2] * result[2][0]);
            result[3][1] =
                -(m[3][0] * result[0][1] + m[3][1] * result[1][1] + m[3][2] * result[2][1]);
            result[3][2] =
                -(m[3][0] * result[0][2] + m[3][1] * result[1][2] + m[3][2] * result[2][2]);
            result[3][3] = 1.0;

            return result;
        }
        else
        {
            // TODO: Compute 2x2 and 3x3 matrix inverse.
        }
    }

    template <>
    inline static float4x4_simd inverse(const float4x4_simd& m)
    {
        // TODO: Calculate simd version 4x4 matrix inverse.
        return float4x4_simd{};
    }

    template <square_matrix M>
    inline static M identity()
    {
        static const M result = make_identity<M>();
        return result;
    }

    template <>
    inline static float4x4_simd identity()
    {
        return float4x4_simd{simd::get_identity_row<0>(),
                             simd::get_identity_row<1>(),
                             simd::get_identity_row<2>(),
                             simd::get_identity_row<3>()};
    }

private:
    template <square_matrix M>
    inline static M make_identity()
    {
        M result = {};
        for (size_t i = 0; i < packed_trait<M>::row_size; ++i)
            result[i][i] = packed_trait<M>::value_type(1);
        return result;
    }
};
} // namespace ash::math