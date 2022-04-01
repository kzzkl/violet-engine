#include "test_common.hpp"

using namespace ash::math;

namespace ash::test
{
namespace math_plain
{
TEST_CASE("vector_plain::add", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    float4 b = {1.0f, 3.0f, 5.0f, 7.0f};

    CHECK(equal(vector::add(a, b), float4{2.0f, 5.0f, 8.0f, 11.0f}));
}

TEST_CASE("vector_plain::sub", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    float4 b = {1.0f, 3.0f, 5.0f, 7.0f};

    CHECK(equal(vector::sub(a, b), float4{0.0f, -1.0f, -2.0f, -3.0f}));
}

TEST_CASE("vector_plain::dot", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    float4 b = {1.0f, 3.0f, 5.0f, 7.0f};

    CHECK(equal(vector::dot(a, b), 50.0f));
}

TEST_CASE("vector_plain::cross", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    float4 b = {3.0f, 4.0f, 5.0f, 0.0f};
    CHECK(equal(vector::cross(a, b), float4{-2.0f, 4.0f, -2.0f, 0.0f}));
}

TEST_CASE("vector_plain::scale", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(equal(vector::scale(a, 2.0f), float4{2.0f, 4.0f, 6.0f, 8.0f}));
}

TEST_CASE("vector_plain::length", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    CHECK(equal(vector::length(a), 3.7416573f)); // sqrt(1^2 + 2^2 + 3^2) = 3.7416573;
}

TEST_CASE("vector_plain::normalize", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    CHECK(equal(vector::normalize(a), float4{0.267261237f, 0.534522474f, 0.801783681f, 0.0f}));
}

TEST_CASE("vector_plain::sqrt", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    CHECK(equal(vector::sqrt(a), float4{1.0f, 1.414213562f, 1.732050807f, 0.0f}));
}

TEST_CASE("vector_plain::reciprocal_sqrt", "[vector]")
{
    float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(equal(vector::reciprocal_sqrt(a), float4{1.0f, 0.707106781f, 0.577350269f, 0.5f}));
}
} // namespace math_plain

namespace math_simd
{
TEST_CASE("vector_simd::add", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    float4 result;
    simd::store(vector::add(a, b), result);

    CHECK(equal(result, float4{4.0f, 6.0f, 8.0f, 10.0f}));
}

TEST_CASE("vector_simd::sub", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    float4 result;
    simd::store(vector::sub(a, b), result);

    CHECK(equal(result, float4{-2.0f, -2.0f, -2.0f, -2.0f}));
}

TEST_CASE("vector_simd::dot", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    CHECK(equal(vector::dot_v(a, b), simd::set(50.0f)));
}

TEST_CASE("vector_simd::cross", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    float4_simd b = simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    float4 result;
    simd::store(vector::cross(a, b), result);
    CHECK(equal(result, float4{-2.0f, 4.0f, -2.0f, 0.0f}));
}

TEST_CASE("vector_simd::scale", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);

    float4 result;
    simd::store(vector::scale(a, 0.5f), result);
    CHECK(equal(result, float4{0.5f, 1.0f, 1.5f, 2.0f}));
}

TEST_CASE("vector_simd::length", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(vector::length_v(a), simd::set(3.7416573f))); // sqrt(1^2 + 2^2 + 3^2) = 3.7416573;
}

TEST_CASE("vector_simd::normalize", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(vector::normalize(a), simd::set(0.267261237f, 0.534522474f, 0.801783681f, 0.0f)));
}

TEST_CASE("vector_simd::sqrt", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(vector::sqrt(a), simd::set(1.0f, 1.414213562f, 1.732050807f, 0.0f)));
}

TEST_CASE("vector_simd::reciprocal_sqrt", "[vector][simd]")
{
    float4_simd a = simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    CHECK(equal(vector::reciprocal_sqrt(a), simd::set(1.0f, 0.707106781f, 0.577350269f, 0.5f)));
}
} // namespace math_simd
} // namespace ash::test