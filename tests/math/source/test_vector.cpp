#include "test_common.hpp"
#include "test_data.hpp"
#include <chrono>
#include <iostream>

namespace violet::test
{
TEST_CASE("vector::load", "[vector]")
{
    float2 r2 = {1.0f, 2.0f};
    CHECK(equal(vector::load(r2), vector::set(1.0f, 2.0f)));

    float3 r3 = {1.0f, 2.0f, 3.0f};
    CHECK(equal(vector::load(r3), vector::set(1.0f, 2.0f, 3.0f, 0.0f)));
    CHECK(equal(vector::load(r3, 4.0f), vector::set(1.0f, 2.0f, 3.0f, 4.0f)));

    float4 r4 = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(equal(vector::load(r4), vector::set(1.0f, 2.0f, 3.0f, 4.0f)));

    float4_align r4a = {1.0f, 2.0f, 3.0f, 4.0f};
    CHECK(equal(vector::load(r4a), vector::set(1.0f, 2.0f, 3.0f, 4.0f)));
}

TEST_CASE("vector::store", "[vector]")
{
    vector4 v = vector::set(1.0f, 2.0f, 3.0f, 4.0f);

    float2 r2;
    vector::store(v, r2);
    CHECK(equal(r2, float2{1.0f, 2.0f}));

    float3 r3;
    vector::store(v, r3);
    CHECK(equal(r3, float3{1.0f, 2.0f, 3.0f}));

    float4 r4;
    vector::store(v, r4);
    CHECK(equal(r4, float4{1.0f, 2.0f, 3.0f, 4.0f}));

    float4_align r4a;
    vector::store(v, r4a);
    CHECK(equal(r4a, float4{1.0f, 2.0f, 3.0f, 4.0f}));
}

TEST_CASE("vector::add", "[vector]")
{
    vector4 a = vector::load(test_data::vec4_a);
    vector4 b = vector::load(test_data::vec4_b);

    CHECK(equal(vector::add(a, b), vector::set(2.0f, 5.0f, 8.0f, 11.0f)));
}

TEST_CASE("vector::sub", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    vector4 b = vector::set(1.0f, 3.0f, 5.0f, 7.0f);

    CHECK(equal(vector::sub(a, b), vector::set(0.0f, -1.0f, -2.0f, -3.0f)));
}

TEST_CASE("vector::mul", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    CHECK(equal(vector::mul(a, 2.0f), vector::set(2.0f, 4.0f, 6.0f, 8.0f)));
}

TEST_CASE("vector::dot", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    vector4 b = vector::set(1.0f, 3.0f, 5.0f, 7.0f);

    CHECK(equal(vector::dot(a, b), 50.0f));
}

TEST_CASE("vector::cross", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 0.0f);
    vector4 b = vector::set(3.0f, 4.0f, 5.0f, 0.0f);
    CHECK(equal(vector::cross(a, b), vector::set(-2.0f, 4.0f, -2.0f, 0.0f)));
}

TEST_CASE("vector::lerp", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 0.0f);
    vector4 b = vector::set(3.0f, 4.0f, 5.0f, 0.0f);
    CHECK(equal(vector::lerp(a, b, 0.5f), vector::set(2.0f, 3.0f, 4.0f, 0.0f)));
}

TEST_CASE("vector::length", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(vector::length(a), 3.7416573f)); // sqrt(1^2 + 2^2 + 3^2) = 3.7416573;
}

TEST_CASE("vector::normalize", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(vector::normalize(a), vector::set(0.267261237f, 0.534522474f, 0.801783681f, 0.0f)));
}

TEST_CASE("vector::sqrt", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 0.0f);
    CHECK(equal(vector::sqrt(a), vector::set(1.0f, 1.414213562f, 1.732050807f, 0.0f)));
}

TEST_CASE("vector::reciprocal_sqrt", "[vector]")
{
    vector4 a = vector::set(1.0f, 2.0f, 3.0f, 4.0f);
    CHECK(equal(vector::reciprocal_sqrt(a), vector::set(1.0f, 0.707106781f, 0.577350269f, 0.5f)));
}

/*TEST_CASE("vector::benchmark", "[vector]")
{
    vector4 data = {};
    vector4 add = {1.0f, 2.0f, 3.0f, 4.0f};

    auto t1 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < 100000000; ++i)
    {
        vector4 v = simd::load(data);
        vector4 a = simd::load(add);

        v = vector::add(v, a);
        v = vector::normalize(v);
        v = vector::mul(v, a);
        v = vector::normalize(v);
        simd::store(v, data);
    }
    auto t2 = std::chrono::steady_clock::now();
    std::cout << data[0] << " " << data[1] << " " << data[2] << " " << data[3]
        << std::endl;

    data = {};

    auto t3 = std::chrono::steady_clock::now();
    vector4 d = simd::load(data);
    vector4 a = simd::load(add);
    for (std::size_t i = 0; i < 100000000; ++i) {
        d = vector::add(d, a);
        d = vector::normalize(d);
        d = vector::mul(d, a);
        d = vector::normalize(d);
    }
    simd::store(d, data);
    auto t4 = std::chrono::steady_clock::now();

    std::cout << (t2 - t1).count() / 1000000000.0f << " " << (t4 - t3).count() / 1000000000.0f
              << std::endl;

    std::cout << data[0] << " " << data[1] << " " << data[2] << " " << data[3]
        << std::endl;
}*/
} // namespace violet::test