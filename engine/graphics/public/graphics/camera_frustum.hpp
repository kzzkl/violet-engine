#pragma once

#include "math/math.hpp"
#include <array>

namespace violet::graphics
{
struct camera_frustum
{
public:
    static inline std::array<math::float4, 6> planes(const math::float4x4& projection)
    {
        math::float4_simd x =
            math::simd::set(projection[0][0], projection[1][0], projection[2][0], projection[3][0]);
        math::float4_simd y =
            math::simd::set(projection[0][1], projection[1][1], projection[2][1], projection[3][1]);
        math::float4_simd z =
            math::simd::set(projection[0][2], projection[1][2], projection[2][2], projection[3][2]);
        math::float4_simd w =
            math::simd::set(projection[0][3], projection[1][3], projection[2][3], projection[3][3]);

        std::array<math::float4, 6> planes;

        math::float4_simd t = math::vector_simd::add(w, x);
        math::float4_simd lenght = math::vector_simd::length_vec3_v(t);
        math::simd::store(math::vector_simd::div(t, lenght), planes[0]);

        t = math::vector_simd::sub(w, x);
        lenght = math::vector_simd::length_vec3_v(t);
        math::simd::store(math::vector_simd::div(t, lenght), planes[1]);

        t = math::vector_simd::add(w, y);
        lenght = math::vector_simd::length_vec3_v(t);
        math::simd::store(math::vector_simd::div(t, lenght), planes[2]);

        t = math::vector_simd::sub(w, y);
        lenght = math::vector_simd::length_vec3_v(t);
        math::simd::store(math::vector_simd::div(t, lenght), planes[3]);

        t = z;
        lenght = math::vector_simd::length_vec3_v(t);
        math::simd::store(math::vector_simd::div(t, lenght), planes[4]);

        t = math::vector_simd::sub(w, z);
        lenght = math::vector_simd::length_vec3_v(t);
        math::simd::store(math::vector_simd::div(t, lenght), planes[5]);

        return planes;
    }

    static inline std::array<math::float3, 8> vertices(const math::float4x4& projection)
    {
        math::float4x4_simd vp_inverse = math::matrix_simd::inverse(math::simd::load(projection));

        std::array<math::float3, 8> vertices;
        for (std::size_t i = 0; i < 8; ++i)
        {
            math::float4_simd v = math::simd::load(ndc[i]);
            v = math::matrix_simd::mul(v, vp_inverse);
            math::float4_simd w = math::simd::replicate<3>(v);
            v = math::vector_simd::div(v, w);
            math::simd::store(v, vertices[i]);
        }

        return vertices;
    }

    static inline std::array<math::float4, 8> vertices_vec4(const math::float4x4& projection)
    {
        math::float4x4_simd vp_inverse = math::matrix_simd::inverse(math::simd::load(projection));

        std::array<math::float4, 8> vertices;
        for (std::size_t i = 0; i < 8; ++i)
        {
            math::float4_simd v = math::simd::load(ndc[i]);
            v = math::matrix_simd::mul(v, vp_inverse);
            math::float4_simd w = math::simd::replicate<3>(v);
            v = math::vector_simd::div(v, w);
            math::simd::store(v, vertices[i]);
        }

        return vertices;
    }

private:
    static constexpr std::array<math::float4_align, 8> ndc = {
        math::float4_align{-1.0f, -1.0f, 0.0f, 1.0f},
        math::float4_align{1.0f,  -1.0f, 0.0f, 1.0f},
        math::float4_align{-1.0f, 1.0f,  0.0f, 1.0f},
        math::float4_align{1.0f,  1.0f,  0.0f, 1.0f},
        math::float4_align{-1.0f, -1.0f, 1.0f, 1.0f},
        math::float4_align{1.0f,  -1.0f, 1.0f, 1.0f},
        math::float4_align{-1.0f, 1.0f,  1.0f, 1.0f},
        math::float4_align{1.0f,  1.0f,  1.0f, 1.0f}
    };
};
} // namespace violet::graphics