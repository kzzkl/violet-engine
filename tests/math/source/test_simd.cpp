#include "test_common.hpp"

using namespace violet::math;

namespace violet::test
{
TEST_CASE("simd::mask", "[simd]")
{
    auto v1 = simd::mask_v<1, 1, 0, 1>;
    auto v2 = simd::set(0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF);

    float4 f1;
    simd::store(v1, f1);

    float4 f2;
    simd::store(v2, f2);
}

TEST_CASE("simd::shuffle", "[simd]")
{
    auto v = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    auto v2 = simd::shuffle<1, 1, 2, 2>(v);

    float4 result;
    simd::store(v2, result);

    CHECK(equal(result, float4{2.0f, 2.0f, 3.0f, 3.0f}));
}

TEST_CASE("simd::store", "[simd]")
{
    float4_simd v = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4 r4;
    simd::store(v, r4);

    CHECK(equal(r4, float4{1.0f, 2.0f, 3.0f, 4.0f}));

    float3 r3;
    simd::store(v, r3);
    CHECK(equal(r3, float3{1.0f, 2.0f, 3.0f}));
}
} // namespace violet::test