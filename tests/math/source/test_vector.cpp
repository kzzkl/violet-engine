#include "test_common.hpp"

using namespace ash::math;

namespace ash::test
{
TEST_CASE("vector add", "[vector]")
{
    int2 a = {1, 2};
    int2 b = {1, 3};

    int2 c = vector::add(a, b);

    CHECK(c[0] == 2);
    CHECK(c[1] == 5);
}

TEST_CASE("simd vector add", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    float4 result;
    simd::store(vector::add(a, b), result);

    CHECK(equal(result, float4{4.0f, 6.0f, 8.0f, 10.0f}));
}

TEST_CASE("vector sub", "[vector]")
{
    int2 a = {1, 2};
    int2 b = {1, 3};

    int2 c = vector::sub(a, b);

    CHECK(c[0] == 0);
    CHECK(c[1] == -1);
}

TEST_CASE("simd vector sub", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    float4 result;
    simd::store(vector::sub(a, b), result);

    CHECK(equal(result, float4{-2.0f, -2.0f, -2.0f, -2.0f}));
}

TEST_CASE("vector dot", "[vector]")
{
    int4 a = {1, 2, 3, 4};
    int4 b = {3, 4, 5, 6};
    CHECK(vector::dot(a, b) == 50);
}

TEST_CASE("simd vector dot", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    CHECK(equal(vector::dot(a, b), 50.0f));

    float4 result;
    simd::store(vector::dot_vector(a, b), result);
    CHECK(equal(result, float4{50.0f, 50.0f, 50.0f, 50.0f}));
}

TEST_CASE("vector cross", "[vector]")
{
    int3 a = {1, 2, 3};
    int3 b = {3, 4, 5};
    CHECK(vector::cross(a, b) == int3{-2, 4, -2});
}

TEST_CASE("simd vector cross", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    float4 result;
    simd::store(vector::cross(a, b), result);
    CHECK(equal(result, float4{-2.0f, 4.0f, -2.0f, 0.0f}));
}

TEST_CASE("vector scale", "[vector]")
{
    int3 a = {1, 2, 3};
    CHECK(vector::scale(a, 2) == int3{2, 4, 6});
}

TEST_CASE("simd vector scale", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);

    float4 result;
    simd::store(vector::scale(a, 0.5f), result);
    CHECK(equal(result, float4{0.5f, 1.0f, 1.5f, 2.0f}));
}

TEST_CASE("vector length", "[vector]")
{
    int3 a = {1, 2, 3};
    CHECK(equal(vector::length(a), 3.7416573f)); // sqrt(1^2 + 2^2 + 3^2) = 3.7416573;
}

TEST_CASE("simd vector length", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(vector::length(a), 3.7416573f)); // sqrt(1^2 + 2^2 + 3^2) = 3.7416573;

    float4 result;
    simd::store(vector::length_vector(a), result);
    CHECK(equal(result, float4{3.7416573f, 3.7416573f, 3.7416573f, 3.7416573f}));
}
} // namespace ash::test