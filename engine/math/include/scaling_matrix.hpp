#pragma once

#include "matrix.hpp"

namespace ash::math
{
struct scaling_matrix : matrix
{
    template <matrix_4x4 M>
    inline static M make(float x, float y, float z)
    {
        return M{x,
                 0.0f,
                 0.0f,
                 0.0f,
                 0.0f,
                 y,
                 0.0f,
                 0.0f,
                 0.0f,
                 0.0f,
                 z,
                 0.0f,
                 0.0f,
                 0.0f,
                 0.0f,
                 1.0f};
    }

    template <>
    inline static float4x4_simd make(float x, float y, float z)
    {
        return float4x4_simd{_mm_setr_ps(x, 0.0f, 0.0f, 0.0f),
                             _mm_setr_ps(0.0f, y, 0.0f, 0.0f),
                             _mm_setr_ps(0.0f, 0.0f, z, 0.0f),
                             simd::get_identity_row<3>()};
    }

    inline static float4x4_simd make(const float4_simd& v)
    {
        return float4x4_simd{_mm_and_ps(v, simd::get_mask<0x1000>()),
                             _mm_and_ps(v, simd::get_mask<0x0100>()),
                             _mm_and_ps(v, simd::get_mask<0x0010>()),
                             simd::get_identity_row<3>()};
    }
};
} // namespace ash::math