#include "test_common.hpp"

using namespace ash::math;

namespace ash::test
{
TEST_CASE("vector::add", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    math::float4 b = {1.0f, 3.0f, 5.0f, 7.0f};

    CHECK(equal(math::vector::add(a, b), math::float4{2.0f, 5.0f, 8.0f, 11.0f}));
}

TEST_CASE("vector::sub", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    math::float4 b = {1.0f, 3.0f, 5.0f, 7.0f};

    CHECK(equal(math::vector::sub(a, b), math::float4{0.0f, -1.0f, -2.0f, -3.0f}));
}

TEST_CASE("vector::dot", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    math::float4 b = {1.0f, 3.0f, 5.0f, 7.0f};

    CHECK(equal(math::vector::dot(a, b), 50.0f));
}

TEST_CASE("vector::cross", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    math::float4 b = {3.0f, 4.0f, 5.0f, 0.0f};
    CHECK(equal(math::vector::cross(a, b), math::float4{-2.0f, 4.0f, -2.0f, 0.0f}));
}

TEST_CASE("vector::scale", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(equal(math::vector::scale(a, 2.0f), math::float4{2.0f, 4.0f, 6.0f, 8.0f}));
}

TEST_CASE("vector::length", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    CHECK(equal(math::vector::length(a), 3.7416573f)); // sqrt(1^2 + 2^2 + 3^2) = 3.7416573;
}

TEST_CASE("vector::normalize", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    CHECK(equal(
        math::vector::normalize(a),
        math::float4{0.267261237f, 0.534522474f, 0.801783681f, 0.0f}));
}

TEST_CASE("vector::sqrt", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 0.0f};
    CHECK(equal(math::vector::sqrt(a), math::float4{1.0f, 1.414213562f, 1.732050807f, 0.0f}));
}

TEST_CASE("vector::reciprocal_sqrt", "[vector]")
{
    math::float4 a = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(equal(
        math::vector::reciprocal_sqrt(a),
        math::float4{1.0f, 0.707106781f, 0.577350269f, 0.5f}));
}

TEST_CASE("vector_simd::add", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    math::float4_simd b = math::simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    math::float4 result;
    math::simd::store(math::vector_simd::add(a, b), result);

    CHECK(equal(result, math::float4{4.0f, 6.0f, 8.0f, 10.0f}));
}

TEST_CASE("vector_simd::sub", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    math::float4_simd b = math::simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    math::float4 result;
    simd::store(math::vector_simd::sub(a, b), result);

    CHECK(equal(result, math::float4{-2.0f, -2.0f, -2.0f, -2.0f}));
}

TEST_CASE("vector_simd::dot", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    math::float4_simd b = math::simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    CHECK(equal(math::vector_simd::dot_v(a, b), math::simd::set(50.0f)));
}

TEST_CASE("vector_simd::cross", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    math::float4_simd b = math::simd::set(3.0f, 4.0f, 5.0f, 6.0f);

    math::float4 result;
    math::simd::store(math::vector_simd::cross(a, b), result);
    CHECK(equal(result, math::float4{-2.0f, 4.0f, -2.0f, 0.0f}));
}

TEST_CASE("vector_simd::scale", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);

    math::float4 result;
    simd::store(math::vector_simd::scale(a, 0.5f), result);
    CHECK(equal(result, math::float4{0.5f, 1.0f, 1.5f, 2.0f}));
}

TEST_CASE("vector_simd::length", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(
        math::vector_simd::length_v(a),
        math::simd::set(3.7416573f))); // sqrt(1^2 + 2^2 + 3^2) = 3.7416573;
}

TEST_CASE("vector_simd::normalize", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(
        math::vector_simd::normalize(a),
        math::simd::set(0.267261237f, 0.534522474f, 0.801783681f, 0.0f)));
}

TEST_CASE("vector_simd::sqrt", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(
        equal(math::vector_simd::sqrt(a), math::simd::set(1.0f, 1.414213562f, 1.732050807f, 0.0f)));
}

TEST_CASE("vector_simd::reciprocal_sqrt", "[vector][simd]")
{
    math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);
    CHECK(equal(
        math::vector_simd::reciprocal_sqrt(a),
        math::simd::set(1.0f, 0.707106781f, 0.577350269f, 0.5f)));
}
} // namespace ash::test