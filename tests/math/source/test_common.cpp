#include "test_common.hpp"

#define VALUE_WITH_MARGIN(v) Catch::Approx(v).margin(0.0005)

namespace violet::test
{
bool equal(float a, float b)
{
    return VALUE_WITH_MARGIN(a) == b;
}

bool equal(const violet::float2& a, const violet::float2& b)
{
    for (std::size_t i = 0; i < 2; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::float3& a, const violet::float3& b)
{
    for (std::size_t i = 0; i < 3; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::float4& a, const violet::float4& b)
{
    for (std::size_t i = 0; i < 4; ++i)
    {
        if (VALUE_WITH_MARGIN(a[i]) != b[i])
            return false;
    }
    return true;
}

bool equal(const violet::float4x4& a, const violet::float4x4& b)
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

#ifdef VIOLET_USE_SIMD
bool equal(const violet::vector4& a, const violet::vector4& b)
{
    float4 va = math::store<float4>(a);
    float4 vb = math::store<float4>(b);

    return equal(va, vb);
}

bool equal(const violet::matrix4& a, const violet::matrix4& b)
{
    float4x4 ma = math::store<float4x4>(a);
    float4x4 mb = math::store<float4x4>(b);

    return equal(ma, mb);
}
#endif
} // namespace violet::test