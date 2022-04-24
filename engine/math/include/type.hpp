#pragma once

#include <cstddef>
#include <cstdint>

namespace ash::math
{
template <typename T, std::size_t S>
struct packed
{
    using value_type = T;

    inline value_type& operator[](std::size_t index) { return this->data[index]; }
    inline const value_type& operator[](std::size_t index) const { return this->data[index]; }

    value_type data[S];
};

using int2 = packed<std::int32_t, 2>;
using int3 = packed<std::int32_t, 3>;
using int4 = packed<std::int32_t, 4>;

using uint2 = packed<std::uint32_t, 2>;
using uint3 = packed<std::uint32_t, 3>;
using uint4 = packed<std::uint32_t, 4>;

using float2 = packed<float, 2>;
using float3 = packed<float, 3>;
using float4 = packed<float, 4>;

struct alignas(16) float4_align : float4
{
};

struct float4x4
{
    using row_type = float4;

    inline row_type& operator[](std::size_t index) { return row[index]; }
    inline const row_type& operator[](std::size_t index) const { return row[index]; }

    row_type row[4];
};

struct alignas(16) float4x4_align : float4x4
{
};
} // namespace ash::math