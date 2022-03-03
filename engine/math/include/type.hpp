#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace ash::math
{
template <typename T, std::size_t Size>
struct pack_1d
{
    using value_type = T;
    using self_type = pack_1d<T, Size>;

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

using int2 = pack_1d<int32_t, 2>;
using int3 = pack_1d<int32_t, 3>;
using int4 = pack_1d<int32_t, 4>;

using uint2 = pack_1d<uint32_t, 2>;
using uint3 = pack_1d<uint32_t, 3>;
using uint4 = pack_1d<uint32_t, 4>;

using float2 = pack_1d<float, 2>;
using float3 = pack_1d<float, 3>;
using float4 = pack_1d<float, 4>;

struct alignas(16) float4_align : public float4
{
};

template <typename T, std::size_t Row, std::size_t Col>
struct pack_2d
{
    using value_type = T;
    using self_type = pack_2d<T, Row, Col>;
    using row_type = pack_1d<T, Col>;

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

using int2x2 = pack_2d<int32_t, 2, 2>;
using int2x3 = pack_2d<int32_t, 2, 3>;
using int3x2 = pack_2d<int32_t, 3, 2>;
using int3x3 = pack_2d<int32_t, 3, 3>;
using int4x4 = pack_2d<int32_t, 4, 4>;

using uint2x2 = pack_2d<uint32_t, 2, 2>;
using uint2x3 = pack_2d<uint32_t, 2, 3>;
using uint3x2 = pack_2d<uint32_t, 3, 2>;
using uint3x3 = pack_2d<uint32_t, 3, 3>;
using uint4x4 = pack_2d<uint32_t, 4, 4>;

using float2x2 = pack_2d<float, 2, 2>;
using float2x3 = pack_2d<float, 2, 3>;
using float3x2 = pack_2d<float, 3, 2>;
using float3x3 = pack_2d<float, 3, 3>;
using float4x4 = pack_2d<float, 4, 4>;

struct alignas(16) float4x4_align : public float4x4
{
};

template <typename T>
struct pack_trait
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
struct is_pack_1d : is_any_of<T, int2, int3, int4, uint2, uint3, uint4, float2, float3, float4>
{
};

template <typename T>
struct is_pack_2d : is_any_of<
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
struct is_row_size_equal : std::bool_constant<pack_trait<T>::row_size == Size>
{
};

template <typename T, std::size_t Size>
struct is_col_size_equal : std::bool_constant<pack_trait<T>::col_size == Size>
{
};

template <typename T1, typename T2>
concept row_size_equal = pack_trait<T1>::row_size == pack_trait<T2>::row_size;

template <typename T1, typename T2>
concept col_size_equal = pack_trait<T1>::col_size == pack_trait<T2>::col_size;

template <typename M>
struct is_square : std::bool_constant<pack_trait<M>::row_size == pack_trait<M>::col_size>
{
};

template <typename M>
concept square_matrix = is_square<M>::value;

template <typename M>
concept matrix_4x4 = is_square<M>::value&& pack_trait<M>::row_size == 4;

template <typename T>
concept row_vector = is_pack_1d<T>::value;

template <typename T>
concept greater_than_1x3 = pack_trait<T>::col_size >= 3;
} // namespace ash::math