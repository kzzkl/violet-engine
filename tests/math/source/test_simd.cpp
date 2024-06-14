#include "test_common.hpp"

namespace violet::test
{
#ifdef VIOLET_USE_SIMD
TEST_CASE("simd::mask", "")
{
    auto v1 = simd::mask_v<1, 1, 0, 1>;
    auto v2 = vector::set(0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF);

    float4 f1 = math::store<float4>(v1);
    float4 f2 = math::store<float4>(v2);
}

TEST_CASE("simd::shuffle", "")
{
    auto v = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    auto v2 = simd::shuffle<1, 1, 2, 2>(v);

    float4 result = math::store<float4>(v2);

    CHECK(equal(result, float4{2.0f, 2.0f, 3.0f, 3.0f}));
}

TEST_CASE("simd::store", "")
{
    vector4 v = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4 r4;
    math::store(v, r4);

    CHECK(equal(r4, float4{1.0f, 2.0f, 3.0f, 4.0f}));

    float3 r3;
    math::store(v, r3);
    CHECK(equal(r3, float3{1.0f, 2.0f, 3.0f}));
}
#endif
} // namespace violet::test