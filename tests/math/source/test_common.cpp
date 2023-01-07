#include "test_common.hpp"

#define VALUE_WITH_MARGIN(v) Catch::Approx(v).margin(0.00000005)

using namespace violet::math;

namespace violet::test
{
bool equal(float a, float b)
{
    return VALUE_WITH_MARGIN(a) == b;
}

bool equal(const violet::math::float2& a, const violet::math::float2& b)
{
    for (std::size_t i = 0; i < 2; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::math::float3& a, const violet::math::float3& b)
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::math::float4& a, const violet::math::float4& b)
{
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::math::float4_simd& a, const violet::math::float4_simd& b)
{
    float4 va, vb;
    simd::store(a, va);
    simd::store(b, vb);

    return equal(va, vb);
}

bool equal(const violet::math::float4x4& a, const violet::math::float4x4& b)
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

bool equal(const violet::math::float4x4_simd& a, const violet::math::float4x4_simd& b)
{
    float4x4 ma, mb;
    simd::store(a, ma);
    simd::store(b, mb);

    return equal(ma, mb);
}
} // namespace violet::test