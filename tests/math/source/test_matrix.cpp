#include "test_common.hpp"

using namespace ash::math;

namespace ash::test
{
TEST_CASE("matrix mul", "[matrix]")
{
    // 3x3 3x3
    {
        float3x3 a = {1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f};
        float3x3 b = {1.0f, 2.0f, 3.0f, 4.0f, 1.0f, 2.0f, 3.0f, 4.0f, 1.0f};
        float3x3 result = matrix::mul(a, b);

        CHECK(
            equal(result, float3x3{18.0f, 16.0f, 10.0f, 14.0f, 17.0f, 16.0f, 22.0f, 14.0f, 18.0f}));
    }

    // 3x2 2x3
    {
        float3x2 a = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
        float2x3 b = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

        float3x3 result = matrix::mul(a, b);
        CHECK(
            equal(result, float3x3{9.0f, 12.0f, 15.0f, 19.0f, 26.0f, 33.0f, 29.0f, 40.0f, 51.0f}));
    }
}

TEST_CASE("simd matrix mul", "[matrix][simd]")
{
    float4x4_simd a = simd::set(
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        5.0f,
        6.0f,
        7.0f,
        8.0f,
        9.0f,
        10.0f,
        11.0f,
        12.0f,
        13.0f,
        14.0f,
        15.0f,
        16.0f);

    float4x4_simd b = simd::set(
        1.0f,
        2.0f,
        3.1f,
        4.2f,
        5.3f,
        6.4f,
        7.5f,
        8.6f,
        9.7f,
        0.8f,
        1.9f,
        2.1f,
        3.2f,
        4.3f,
        5.4f,
        6.5f);

    float4x4 result;
    simd::store(matrix::mul(a, b), result);

    CHECK(equal(
        result,
        float4x4{53.5000000f,
                 34.4000015f,
                 45.3999977f,
                 53.7000008f,
                 130.300003f,
                 88.4000015f,
                 117.000000f,
                 139.300003f,
                 207.100006f,
                 142.400009f,
                 188.600006f,
                 224.899994f,
                 283.900024f,
                 196.399994f,
                 260.200012f,
                 310.500000f}));
}

TEST_CASE("simd matrix scale", "[matrix]")
{
    float4x4_simd a = simd::set(
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        5.0f,
        6.0f,
        7.0f,
        8.0f,
        9.0f,
        10.0f,
        11.0f,
        12.0f,
        13.0f,
        14.0f,
        15.0f,
        16.0f);

    float4x4 result;
    simd::store(matrix::scale(a, 0.5f), result);

    CHECK(equal(
        result,
        float4x4{0.5f,
                 1.0f,
                 1.5f,
                 2.0f,
                 2.5f,
                 3.0f,
                 3.5f,
                 4.0f,
                 4.5f,
                 5.0f,
                 5.5f,
                 6.0f,
                 6.5f,
                 7.0f,
                 7.5f,
                 8.0f}));
}

TEST_CASE("matrix transpose", "[matrix]")
{
    float2x3 a = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    float3x2 result = matrix::transpose(a);

    CHECK(equal(result, float3x2{1.0f, 4.0f, 2.0f, 5.0f, 3.0f, 6.0f}));
}

TEST_CASE("simd matrix transpose", "[matrix]")
{
    float4x4_simd a = simd::set(
        1.0f,
        2.0f,
        3.0f,
        4.0f,
        5.0f,
        6.0f,
        7.0f,
        8.0f,
        9.0f,
        10.0f,
        11.0f,
        12.0f,
        13.0f,
        14.0f,
        15.0f,
        16.0f);

    float4x4 result;
    simd::store(matrix::transpose(a), result);

    CHECK(equal(
        result,
        float4x4{1.0f,
                 5.0f,
                 9.0f,
                 13.0f,
                 2.0f,
                 6.0f,
                 10.0f,
                 14.0f,
                 3.0f,
                 7.0f,
                 11.0f,
                 15.0f,
                 4.0f,
                 8.0f,
                 12.0f,
                 16.0f}));
}
} // namespace ash::test