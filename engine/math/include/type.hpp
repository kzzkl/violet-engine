#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ash::math
{
template <typename T, std::size_t Size>
struct packed_1d
{
    using value_type = T;
    using self_type = packed_1d<T, Size>;

    static constexpr std::size_t row_size = 1;
    static constexpr std::size_t col_size = Size;

    inline value_type& operator[](std::size_t index) { return data[index]; }
    inline const value_type& operator[](std::size_t index) const { return data[index]; }

    inline bool operator==(const self_type& other) const requires std::is_integral_v<value_type>
    {
        for (std::size_t i = 0; i < col_size; ++i)
        {
            if (data[i] != other[i])
                return false;
        }
        return true;
    }

    value_type data[Size];
};

using int2 = packed_1d<int32_t, 2>;
using int3 = packed_1d<int32_t, 3>;
using int4 = packed_1d<int32_t, 4>;

using uint2 = packed_1d<uint32_t, 2>;
using uint3 = packed_1d<uint32_t, 3>;
using uint4 = packed_1d<uint32_t, 4>;

using float2 = packed_1d<float, 2>;
using float3 = packed_1d<float, 3>;
using float4 = packed_1d<float, 4>;

struct alignas(16) float4_align : float4
{
};

template <typename T, std::size_t Row, std::size_t Col>
struct packed_2d
{
    using value_type = T;
    using self_type = packed_2d<T, Row, Col>;
    using row_type = packed_1d<T, Col>;

    static constexpr std::size_t row_size = Row;
    static constexpr std::size_t col_size = Col;

    inline row_type& operator[](std::size_t index) { return data[index]; }
    inline const row_type& operator[](std::size_t index) const { return data[index]; }

    inline bool operator==(const self_type& other) const requires std::is_integral_v<value_type>
    {
        for (std::size_t i = 0; i < row_size; ++i)
        {
            for (std::size_t j = 0; j < col_size; ++j)
            {
                if (data[i][j] != other[i][j])
                    return false;
            }
        }
        return true;
    }

    row_type data[Row];
};

using int2x2 = packed_2d<int32_t, 2, 2>;
using int2x3 = packed_2d<int32_t, 2, 3>;
using int3x2 = packed_2d<int32_t, 3, 2>;
using int3x3 = packed_2d<int32_t, 3, 3>;
using int4x4 = packed_2d<int32_t, 4, 4>;

using uint2x2 = packed_2d<uint32_t, 2, 2>;
using uint2x3 = packed_2d<uint32_t, 2, 3>;
using uint3x2 = packed_2d<uint32_t, 3, 2>;
using uint3x3 = packed_2d<uint32_t, 3, 3>;
using uint4x4 = packed_2d<uint32_t, 4, 4>;

using float2x2 = packed_2d<float, 2, 2>;
using float2x3 = packed_2d<float, 2, 3>;
using float3x2 = packed_2d<float, 3, 2>;
using float3x3 = packed_2d<float, 3, 3>;
using float4x4 = packed_2d<float, 4, 4>;

struct alignas(16) float4x4_align : float4x4
{
};

template <typename T>
struct packed_trait
{
    using value_type = T::value_type;

    static constexpr std::size_t row_size = T::row_size;
    static constexpr std::size_t col_size = T::col_size;
};

template <typename T, typename... Types>
struct is_any_of : std::bool_constant<(std::is_same_v<T, Types> || ...)>
{
};

template <typename T>
struct is_packed_1d : is_any_of<T, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4>
{
};

template <typename T>
struct is_packed_2d : is_any_of<
                          T,
                          int2x2,
                          int2x3,
                          int3x2,
                          int3x3,
                          int4x4,
                          uint2x2,
                          uint2x3,
                          uint3x2,
                          uint3x3,
                          uint4x4,
                          float2x2,
                          float2x3,
                          float3x2,
                          float3x3,
                          float4x4>
{
};

template <typename T, std::size_t Size>
struct is_row_size_equal : std::bool_constant<packed_trait<T>::row_size == Size>
{
};

template <typename T, std::size_t Size>
struct is_col_size_equal : std::bool_constant<packed_trait<T>::col_size == Size>
{
};

template <typename T1, typename T2>
concept row_size_equal = packed_trait<T1>::row_size == packed_trait<T2>::row_size;

template <typename T1, typename T2>
concept col_size_equal = packed_trait<T1>::col_size == packed_trait<T2>::col_size;

template <typename M>
struct is_square : std::bool_constant<packed_trait<M>::row_size == packed_trait<M>::col_size>
{
};

template <typename M>
concept square_matrix = is_square<M>::value;

template <typename M>
concept matrix_4x4 = is_square<M>::value&& packed_trait<M>::row_size == 4;

template <typename T>
concept row_vector = is_packed_1d<T>::value;

template <typename T>
concept vector_1x3_1x4 = packed_trait<T>::col_size == 3 || packed_trait<T>::col_size == 4;
} // namespace ash::math