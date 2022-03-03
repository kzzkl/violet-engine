#include "test_common.hpp"

#define VALUE_WITH_MARGIN(v) Approx(v).margin(0.00000005)

namespace ash::test
{
bool equal(float a, float b)
{
    return VALUE_WITH_MARGIN(a) == b;
}

bool equal(const ash::math::float2& a, const ash::math::float2& b)
{
    for (std::size_t i = 0; i < 2; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const ash::math::float3& a, const ash::math::float3& b)
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const ash::math::float4& a, const ash::math::float4& b)
{
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const ash::math::float2x2& a, const ash::math::float2x2& b)
{
    for (std::size_t i = 0; i < 2; ++i)
    {
        for (std::size_t j = 0; j < 2; ++j)
        {
            if (VALUE_WITH_MARGIN(a[i][j]) != b[i][j])
                return false;
        }
    }
    return true;
}

bool equal(const ash::math::float2x3& a, const ash::math::float2x3& b)
{
    for (std::size_t i = 0; i < 2; ++i)
    {
        for (std::size_t j = 0; j < 3; ++j)
        {
            if (VALUE_WITH_MARGIN(a[i][j]) != b[i][j])
                return false;
        }
    }
    return true;
}

bool equal(const ash::math::float3x2& a, const ash::math::float3x2& b)
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 2; ++j)
        {
            if (VALUE_WITH_MARGIN(a[i][j]) != b[i][j])
                return false;
        }
    }
    return true;
}

bool equal(const ash::math::float3x3& a, const ash::math::float3x3& b)
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        for (std::size_t j = 0; j < 3; ++j)
        {
            if (VALUE_WITH_MARGIN(a[i][j]) != b[i][j])
                return false;
        }
    }
    return true;
}

bool equal(const ash::math::float4x4& a, const ash::math::float4x4& b)
{
    for (std::size_t i = 0; i < 4; ++i)
    {
        for (std::size_t j = 0; j < 4; ++j)
        {
            if (VALUE_WITH_MARGIN(a[i][j]) != b[i][j])
                return false;
        }
    }
    return true;
}
} // namespace ash::test