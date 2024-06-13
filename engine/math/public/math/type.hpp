#pragma once

#include <cstddef>
#include <cstdint>

#ifdef VIOLET_USE_SIMD
#    include "math/simd.hpp"
#endif

namespace violet
{
template <typename T>
struct vec2
{
    using value_type = T;

    inline value_type& operator[](std::size_t index) { return this->data[index]; }
    inline const value_type& operator[](std::size_t index) const { return this->data[index]; }

    union {
        struct
        {
            value_type x, y;
        };
        value_type data[2];
    };
};

template <typename T>
struct vec3
{
    using value_type = T;

    inline value_type& operator[](std::size_t index) { return this->data[index]; }
    inline const value_type& operator[](std::size_t index) const { return this->data[index]; }

    union {
        struct
        {
            value_type x, y, z;
        };
        value_type data[3];
    };
};

template <typename T>
struct vec4
{
    using value_type = T;

    inline value_type& operator[](std::size_t index) { return this->data[index]; }
    inline const value_type& operator[](std::size_t index) const { return this->data[index]; }

    union {
        struct
        {
            value_type x, y, z, w;
        };
        value_type data[4];
    };
};

using int2 = vec2<std::int32_t>;
using int3 = vec3<std::int32_t>;
using int4 = vec4<std::int32_t>;

using uint2 = vec2<std::uint32_t>;
using uint3 = vec3<std::uint32_t>;
using uint4 = vec4<std::uint32_t>;

using float2 = vec2<float>;
using float3 = vec3<float>;
using float4 = vec4<float>;
struct alignas(16) float4_align : float4
{
};

template <typename T, std::size_t S>
struct mat
{
    using row_type = T;

    inline row_type& operator[](std::size_t index) { return this->row[index]; }
    inline const row_type& operator[](std::size_t index) const { return this->row[index]; }

    row_type row[S];
};

using float4x3 = mat<float4, 3>;
struct alignas(16) float4x3_align : float4x3
{
};

using float4x4 = mat<float4, 4>;
struct alignas(16) float4x4_align : float4x4
{
};

#ifdef VIOLET_USE_SIMD
using vector4f = __m128;
using vector4 = vector4f;

struct alignas(16) matrix4x4f
{
    using row_type = vector4f;

    inline row_type& operator[](std::size_t index) { return row[index]; }
    inline const row_type& operator[](std::size_t index) const { return row[index]; }

    row_type row[4];
};
using matrix4x4 = matrix4x4f;
using matrix4 = matrix4x4;
#else
using vector4f = float4;
using vector4 = vector4f;

using matrix4x4f = float4x4;
using matrix4x4 = matrix4x4f;
using matrix4 = matrix4x4f;
#endif
} // namespace violet