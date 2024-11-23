#include "test_common.hpp"
#include <chrono>
#include <iostream>

namespace violet::test
{
struct test_data
{
    static constexpr vec4f vec4_a = {1.0f, 2.0f, 3.0f, 4.0f};
    static constexpr vec4f vec4_b = {1.0f, 3.0f, 5.0f, 7.0f};
};

TEST_CASE("vector::add", "[vector]")
{
    vec4f result = {2.0f, 5.0f, 8.0f, 11.0f};

    CHECK(equal(vector::add(test_data::vec4_a, test_data::vec4_b), result));
    CHECK(equal(vector::add(math::load(test_data::vec4_a), math::load(test_data::vec4_b)), result));
}

TEST_CASE("vector::sub", "[vector]")
{
    vec4f result = {0.0f, -1.0f, -2.0f, -3.0f};

    CHECK(equal(vector::sub(test_data::vec4_a, test_data::vec4_b), result));
    CHECK(equal(vector::sub(math::load(test_data::vec4_a), math::load(test_data::vec4_b)), result));
}

TEST_CASE("vector::mul", "[vector]")
{
    vec4f result = {2.0f, 4.0f, 6.0f, 8.0f};

    CHECK(equal(vector::mul(test_data::vec4_a, 2.0f), result));
    CHECK(equal(vector::mul(math::load(test_data::vec4_a), 2.0f), result));
}

TEST_CASE("vector::dot", "[vector]")
{
    float result = 50.0f;

    CHECK(equal(vector::dot(test_data::vec4_a, test_data::vec4_b), result));
    CHECK(equal(vector::dot(math::load(test_data::vec4_a), math::load(test_data::vec4_b)), result));
}

TEST_CASE("vector::cross", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {3.0f, 4.0f, 5.0f};
    vec3f result = {-2.0f, 4.0f, -2.0f};

    CHECK(equal(vector::cross(a, b), result));
    CHECK(equal(vector::cross(math::load(a), math::load(b)), result));
}

TEST_CASE("vector::lerp", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f b = {3.0f, 4.0f, 5.0f, 0.0f};
    vec4f result = {2.0f, 3.0f, 4.0f, 0.0f};

    CHECK(equal(vector::lerp(a, b, 0.5f), result));
    CHECK(equal(vector::lerp(math::load(a), math::load(b), 0.5f), result));
}

TEST_CASE("vector::length", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    float result = 3.7416573f;

    CHECK(equal(vector::length(a), 3.7416573f));
    CHECK(equal(vector::length(math::load(a)), 3.7416573f));
}

TEST_CASE("vector::normalize", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f result = {0.267261237f, 0.534522474f, 0.801783681f, 0.0f};

    CHECK(equal(vector::normalize(a), result));
    CHECK(equal(vector::normalize(math::load(a)), result));
}

TEST_CASE("vector::sqrt", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f result = {1.0f, 1.414213562f, 1.732050807f, 0.0f};

    CHECK(equal(vector::sqrt(a), result));
    CHECK(equal(vector::sqrt(math::load(a)), result));
}

TEST_CASE("vector::reciprocal_sqrt", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f result = {1.0f, 0.707106781f, 0.577350269f, 0.5f};

    CHECK(equal(vector::reciprocal_sqrt(a), result));
    CHECK(equal(vector::reciprocal_sqrt(math::load(a)), result));
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