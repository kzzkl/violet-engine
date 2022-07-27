#pragma once

#include "matrix.hpp"
#include <array>

namespace ash::math
{
struct utility
{
public:
    static inline std::array<float4, 6> frustum_planes(const float4x4& projection)
    {
        float4_simd x =
            simd::set(projection[0][0], projection[1][0], projection[2][0], projection[3][0]);
        float4_simd y =
            simd::set(projection[0][1], projection[1][1], projection[2][1], projection[3][1]);
        float4_simd z =
            simd::set(projection[0][2], projection[1][2], projection[2][2], projection[3][2]);
        float4_simd w =
            simd::set(projection[0][3], projection[1][3], projection[2][3], projection[3][3]);

        std::array<float4, 6> planes;

        float4_simd t = vector_simd::add(w, x);
        float4_simd lenght = vector_simd::length_vec3_v(t);
        simd::store(vector_simd::div(t, lenght), planes[0]);

        t = vector_simd::sub(w, x);
        lenght = vector_simd::length_vec3_v(t);
        simd::store(vector_simd::div(t, lenght), planes[1]);

        t = vector_simd::add(w, y);
        lenght = vector_simd::length_vec3_v(t);
        simd::store(vector_simd::div(t, lenght), planes[2]);

        t = vector_simd::sub(w, y);
        lenght = vector_simd::length_vec3_v(t);
        simd::store(vector_simd::div(t, lenght), planes[3]);

        t = z;
        lenght = vector_simd::length_vec3_v(t);
        simd::store(vector_simd::div(t, lenght), planes[4]);

        t = vector_simd::sub(w, z);
        lenght = vector_simd::length_vec3_v(t);
        simd::store(vector_simd::div(t, lenght), planes[5]);

        return planes;
    }

    static inline std::array<float3, 8> frustum_vertices(const float4x4& projection)
    {
        float4x4_simd vp_inverse = matrix_simd::inverse(simd::load(projection));

        std::array<float3, 8> vertices;
        for (std::size_t i = 0; i < 8; ++i)
        {
            float4_simd v = simd::load(ndc[i]);
            v = matrix_simd::mul(v, vp_inverse);
            float4_simd w = simd::replicate<3>(v);
            v = vector_simd::div(v, w);
            simd::store(v, vertices[i]);
        }

        return vertices;
    }

    static inline std::array<float4, 8> frustum_vertices_vec4(const float4x4& projection)
    {
        float4x4_simd vp_inverse = matrix_simd::inverse(simd::load(projection));

        std::array<float4, 8> vertices;
        for (std::size_t i = 0; i < 8; ++i)
        {
            float4_simd v = simd::load(ndc[i]);
            v = matrix_simd::mul(v, vp_inverse);
            float4_simd w = simd::replicate<3>(v);
            v = vector_simd::div(v, w);
            simd::store(v, vertices[i]);
        }

        return vertices;
    }

private:
    static constexpr std::array<float4_align, 8> ndc = {
        float4_align{-1.0f, -1.0f, 0.0f, 1.0f},
        float4_align{1.0f,  -1.0f, 0.0f, 1.0f},
        float4_align{-1.0f, 1.0f,  0.0f, 1.0f},
        float4_align{1.0f,  1.0f,  0.0f, 1.0f},
        float4_align{-1.0f, -1.0f, 1.0f, 1.0f},
        float4_align{1.0f,  -1.0f, 1.0f, 1.0f},
        float4_align{-1.0f, 1.0f,  1.0f, 1.0f},
        float4_align{1.0f,  1.0f,  1.0f, 1.0f}
    };
};
} // namespace ash::math