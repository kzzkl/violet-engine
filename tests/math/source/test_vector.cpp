#include "math/vector.hpp"
#include "test_common.hpp"

namespace violet::test
{
struct test_data
{
    static constexpr vec4f vec4_a = {1.0f, 2.0f, 3.0f, 4.0f};
    static constexpr vec4f vec4_b = {1.0f, 3.0f, 5.0f, 7.0f};
};

// ==================== Addition Tests ====================

TEST_CASE("vector::add vec4", "[vector]")
{
    vec4f result = {2.0f, 5.0f, 8.0f, 11.0f};

    CHECK(equal(vector::add(test_data::vec4_a, test_data::vec4_b), result));
    CHECK(equal(vector::add(math::load(test_data::vec4_a), math::load(test_data::vec4_b)), result));
}

TEST_CASE("vector::add vec3", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {4.0f, 5.0f, 6.0f};
    vec3f result = {5.0f, 7.0f, 9.0f};

    CHECK(equal(vector::add(a, b), result));
}

TEST_CASE("vector::add vec2", "[vector]")
{
    vec2f a = {1.0f, 2.0f};
    vec2f b = {3.0f, 4.0f};
    vec2f result = {4.0f, 6.0f};

    CHECK(equal(vector::add(a, b), result));
}

// ==================== Subtraction Tests ====================

TEST_CASE("vector::sub vec4", "[vector]")
{
    vec4f result = {0.0f, -1.0f, -2.0f, -3.0f};

    CHECK(equal(vector::sub(test_data::vec4_a, test_data::vec4_b), result));
    CHECK(equal(vector::sub(math::load(test_data::vec4_a), math::load(test_data::vec4_b)), result));
}

TEST_CASE("vector::sub vec3", "[vector]")
{
    vec3f a = {4.0f, 5.0f, 6.0f};
    vec3f b = {1.0f, 2.0f, 3.0f};
    vec3f result = {3.0f, 3.0f, 3.0f};

    CHECK(equal(vector::sub(a, b), result));
}

TEST_CASE("vector::sub vec2", "[vector]")
{
    vec2f a = {4.0f, 6.0f};
    vec2f b = {1.0f, 2.0f};
    vec2f result = {3.0f, 4.0f};

    CHECK(equal(vector::sub(a, b), result));
}

// ==================== Multiplication Tests ====================

TEST_CASE("vector::mul vec4 with scalar", "[vector]")
{
    vec4f result = {2.0f, 4.0f, 6.0f, 8.0f};

    CHECK(equal(vector::mul(test_data::vec4_a, 2.0f), result));
    CHECK(equal(vector::mul(math::load(test_data::vec4_a), 2.0f), result));
}

TEST_CASE("vector::mul vec4 with vector", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {2.0f, 3.0f, 4.0f, 5.0f};
    vec4f result = {2.0f, 6.0f, 12.0f, 20.0f};

    CHECK(equal(vector::mul(a, b), result));
    CHECK(equal(vector::mul(math::load(a), math::load(b)), result));
}

TEST_CASE("vector::mul vec3 with scalar", "[vector]")
{
    vec3f v = {1.0f, 2.0f, 3.0f};
    vec3f result = {2.0f, 4.0f, 6.0f};

    CHECK(equal(vector::mul(v, 2.0f), result));
}

TEST_CASE("vector::mul vec3 with vector", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {2.0f, 3.0f, 4.0f};
    vec3f result = {2.0f, 6.0f, 12.0f};

    CHECK(equal(vector::mul(a, b), result));
}

TEST_CASE("vector::mul vec2 with scalar", "[vector]")
{
    vec2f v = {1.0f, 2.0f};
    vec2f result = {2.0f, 4.0f};

    CHECK(equal(vector::mul(v, 2.0f), result));
}

TEST_CASE("vector::mul vec2 with vector", "[vector]")
{
    vec2f a = {1.0f, 2.0f};
    vec2f b = {2.0f, 3.0f};
    vec2f result = {2.0f, 6.0f};

    CHECK(equal(vector::mul(a, b), result));
}

// ==================== Division Tests ====================

TEST_CASE("vector::div vec4 with vector", "[vector]")
{
    vec4f a = {4.0f, 6.0f, 8.0f, 10.0f};
    vec4f b = {2.0f, 2.0f, 2.0f, 2.0f};
    vec4f result = {2.0f, 3.0f, 4.0f, 5.0f};

    CHECK(equal(vector::div(a, b), result));
}

TEST_CASE("vector::div vec4f_simd", "[vector]")
{
    vec4f a = {4.0f, 6.0f, 8.0f, 10.0f};
    vec4f b = {2.0f, 2.0f, 2.0f, 2.0f};
    vec4f result = {2.0f, 3.0f, 4.0f, 5.0f};

    CHECK(equal(vector::div(math::load(a), math::load(b)), result));
}

TEST_CASE("vector::div vec4 with scalar", "[vector]")
{
    vec4f v = {2.0f, 4.0f, 6.0f, 8.0f};
    vec4f result = {1.0f, 2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::div(v, 2.0f), result));
    CHECK(equal(vector::div(math::load(v), 2.0f), result));
}

TEST_CASE("vector::div vec3 with vector", "[vector]")
{
    vec3f a = {4.0f, 6.0f, 8.0f};
    vec3f b = {2.0f, 2.0f, 2.0f};
    vec3f result = {2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::div(a, b), result));
}

TEST_CASE("vector::div vec3 with scalar", "[vector]")
{
    vec3f v = {2.0f, 4.0f, 6.0f};
    vec3f result = {1.0f, 2.0f, 3.0f};

    CHECK(equal(vector::div(v, 2.0f), result));
}

TEST_CASE("vector::div vec2 with vector", "[vector]")
{
    vec2f a = {4.0f, 6.0f};
    vec2f b = {2.0f, 2.0f};
    vec2f result = {2.0f, 3.0f};

    CHECK(equal(vector::div(a, b), result));
}

TEST_CASE("vector::div vec2 with scalar", "[vector]")
{
    vec2f v = {2.0f, 4.0f};
    vec2f result = {1.0f, 2.0f};

    CHECK(equal(vector::div(v, 2.0f), result));
}

// ==================== Dot Product Tests ====================

TEST_CASE("vector::dot vec4", "[vector]")
{
    float result = 50.0f;

    CHECK(equal(vector::dot(test_data::vec4_a, test_data::vec4_b), result));
    CHECK(equal(vector::dot(math::load(test_data::vec4_a), math::load(test_data::vec4_b)), result));
}

TEST_CASE("vector::dot vec3", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {4.0f, 5.0f, 6.0f};
    float result = 32.0f;

    CHECK(equal(vector::dot(a, b), result));
}

TEST_CASE("vector::dot vec2", "[vector]")
{
    vec2f a = {1.0f, 2.0f};
    vec2f b = {3.0f, 4.0f};
    float result = 11.0f;

    CHECK(equal(vector::dot(a, b), result));
}

// ==================== Cross Product Tests ====================

TEST_CASE("vector::cross vec3", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {3.0f, 4.0f, 5.0f};
    vec3f result = {-2.0f, 4.0f, -2.0f};

    CHECK(equal(vector::cross(a, b), result));
    CHECK(equal(vector::cross(math::load(a), math::load(b)), result));
}

TEST_CASE("vector::cross vec3 orthogonal", "[vector]")
{
    vec3f a = {1.0f, 0.0f, 0.0f};
    vec3f b = {0.0f, 1.0f, 0.0f};
    vec3f result = {0.0f, 0.0f, 1.0f};

    CHECK(equal(vector::cross(a, b), result));
}

TEST_CASE("vector::cross vec2", "[vector]")
{
    vec2f a = {1.0f, 0.0f};
    vec2f b = {0.0f, 1.0f};
    float result = 1.0f;

    CHECK(equal(vector::cross(a, b), result));
}

TEST_CASE("vector::cross vec2 anti-parallel", "[vector]")
{
    vec2f a = {1.0f, 0.0f};
    vec2f b = {1.0f, 0.0f};
    float result = 0.0f;

    CHECK(equal(vector::cross(a, b), result));
}

// ==================== Lerp Tests ====================

TEST_CASE("vector::lerp vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f b = {3.0f, 4.0f, 5.0f, 0.0f};
    vec4f result = {2.0f, 3.0f, 4.0f, 0.0f};

    CHECK(equal(vector::lerp(a, b, 0.5f), result));
    CHECK(equal(vector::lerp(math::load(a), math::load(b), 0.5f), result));
}

TEST_CASE("vector::lerp vec3", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {3.0f, 4.0f, 5.0f};
    vec3f result = {2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::lerp(a, b, 0.5f), result));
}

TEST_CASE("vector::lerp vec2", "[vector]")
{
    vec2f a = {1.0f, 2.0f};
    vec2f b = {3.0f, 4.0f};
    vec2f result = {2.0f, 3.0f};

    CHECK(equal(vector::lerp(a, b, 0.5f), result));
}

TEST_CASE("vector::lerp t=0", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {5.0f, 6.0f, 7.0f, 8.0f};

    CHECK(equal(vector::lerp(a, b, 0.0f), a));
}

TEST_CASE("vector::lerp t=1", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {5.0f, 6.0f, 7.0f, 8.0f};

    CHECK(equal(vector::lerp(a, b, 1.0f), b));
}

// ==================== Length Tests ====================

TEST_CASE("vector::length vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    float result = 3.7416573f;

    CHECK(equal(vector::length(a), result));
    CHECK(equal(vector::length(math::load(a)), result));
}

TEST_CASE("vector::length vec3", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    float result = 3.7416573f;

    CHECK(equal(vector::length(a), result));
}

TEST_CASE("vector::length vec2", "[vector]")
{
    vec2f a = {3.0f, 4.0f};
    float result = 5.0f;

    CHECK(equal(vector::length(a), result));
}

TEST_CASE("vector::length_sq vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    float result = 30.0f;

    CHECK(equal(vector::length_sq(a), result));
}

TEST_CASE("vector::length_sq vec3", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    float result = 14.0f;

    CHECK(equal(vector::length_sq(a), result));
}

TEST_CASE("vector::length_sq vec2", "[vector]")
{
    vec2f a = {3.0f, 4.0f};
    float result = 25.0f;

    CHECK(equal(vector::length_sq(a), result));
}

// ==================== Normalize Tests ====================

TEST_CASE("vector::normalize vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f result = {0.267261237f, 0.534522474f, 0.801783681f, 0.0f};

    CHECK(equal(vector::normalize(a), result));
    CHECK(equal(vector::normalize(math::load(a)), result));
}

TEST_CASE("vector::normalize vec3", "[vector]")
{
    vec3f a = {3.0f, 4.0f, 0.0f};
    vec3f result = {0.6f, 0.8f, 0.0f};

    CHECK(equal(vector::normalize(a), result));
}

TEST_CASE("vector::normalize vec2", "[vector]")
{
    vec2f a = {3.0f, 4.0f};
    vec2f result = {0.6f, 0.8f};

    CHECK(equal(vector::normalize(a), result));
}

TEST_CASE("vector::normalize zero vector vec3", "[vector]")
{
    vec3f zero = {0.0f, 0.0f, 0.0f};

    CHECK(equal(vector::normalize(zero), zero));
}

TEST_CASE("vector::normalize small vector vec3", "[vector]")
{
    vec3f small = {1e-10f, 1e-10f, 1e-10f};

    CHECK(equal(vector::normalize(small), small));
}

// ==================== Sqrt Tests ====================

TEST_CASE("vector::sqrt vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f result = {1.0f, 1.414213562f, 1.732050807f, 0.0f};

    CHECK(equal(vector::sqrt(a), result));
    CHECK(equal(vector::sqrt(math::load(a)), result));
}

// ==================== Reciprocal Sqrt Tests ====================

TEST_CASE("vector::reciprocal_sqrt vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f result = {1.0f, 0.707106781f, 0.577350269f, 0.5f};

    CHECK(equal(vector::reciprocal_sqrt(a), result));
    CHECK(equal(vector::reciprocal_sqrt(math::load(a)), result));
}

// ==================== Min Tests ====================

TEST_CASE("vector::min vec4 single", "[vector]")
{
    vec4f v = {4.0f, 2.0f, 8.0f, 1.0f};
    float result = 1.0f;

    CHECK(equal(vector::min(v), result));
}

TEST_CASE("vector::min vec3 single", "[vector]")
{
    vec3f v = {4.0f, 2.0f, 8.0f};
    float result = 2.0f;

    CHECK(equal(vector::min(v), result));
}

TEST_CASE("vector::min vec2 single", "[vector]")
{
    vec2f v = {4.0f, 2.0f};
    float result = 2.0f;

    CHECK(equal(vector::min(v), result));
}

TEST_CASE("vector::min vec4 pair", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {0.0f, 5.0f, 2.0f, 8.0f};
    vec4f result = {0.0f, 2.0f, 2.0f, 4.0f};

    CHECK(equal(vector::min(a, b), result));
    CHECK(equal(vector::min(math::load(a), math::load(b)), result));
}

TEST_CASE("vector::min vec3 pair", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {0.0f, 5.0f, 2.0f};
    vec3f result = {0.0f, 2.0f, 2.0f};

    CHECK(equal(vector::min(a, b), result));
}

TEST_CASE("vector::min vec2 pair", "[vector]")
{
    vec2f a = {1.0f, 2.0f};
    vec2f b = {0.0f, 5.0f};
    vec2f result = {0.0f, 2.0f};

    CHECK(equal(vector::min(a, b), result));
}

// ==================== Max Tests ====================

TEST_CASE("vector::max vec4 single", "[vector]")
{
    vec4f v = {4.0f, 2.0f, 8.0f, 1.0f};
    float result = 8.0f;

    CHECK(equal(vector::max(v), result));
}

TEST_CASE("vector::max vec3 single", "[vector]")
{
    vec3f v = {4.0f, 2.0f, 8.0f};
    float result = 8.0f;

    CHECK(equal(vector::max(v), result));
}

TEST_CASE("vector::max vec2 single", "[vector]")
{
    vec2f v = {4.0f, 2.0f};
    float result = 4.0f;

    CHECK(equal(vector::max(v), result));
}

TEST_CASE("vector::max vec4 pair", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {0.0f, 5.0f, 2.0f, 8.0f};
    vec4f result = {1.0f, 5.0f, 3.0f, 8.0f};

    CHECK(equal(vector::max(a, b), result));
    CHECK(equal(vector::max(math::load(a), math::load(b)), result));
}

TEST_CASE("vector::max vec3 pair", "[vector]")
{
    vec3f a = {1.0f, 2.0f, 3.0f};
    vec3f b = {0.0f, 5.0f, 2.0f};
    vec3f result = {1.0f, 5.0f, 3.0f};

    CHECK(equal(vector::max(a, b), result));
}

TEST_CASE("vector::max vec2 pair", "[vector]")
{
    vec2f a = {1.0f, 2.0f};
    vec2f b = {0.0f, 5.0f};
    vec2f result = {1.0f, 5.0f};

    CHECK(equal(vector::max(a, b), result));
}

// ==================== Clamp Tests ====================

TEST_CASE("vector::clamp vec3", "[vector]")
{
    vec3f value = {2.0f, 5.0f, 8.0f};
    vec3f min_val = {1.0f, 3.0f, 5.0f};
    vec3f max_val = {3.0f, 7.0f, 9.0f};
    vec3f result = {2.0f, 5.0f, 8.0f};

    CHECK(equal(vector::clamp(value, min_val, max_val), result));
}

TEST_CASE("vector::clamp vec3 below min", "[vector]")
{
    vec3f value = {0.0f, 2.0f, 4.0f};
    vec3f min_val = {1.0f, 3.0f, 5.0f};
    vec3f max_val = {3.0f, 7.0f, 9.0f};
    vec3f result = {1.0f, 3.0f, 5.0f};

    CHECK(equal(vector::clamp(value, min_val, max_val), result));
}

TEST_CASE("vector::clamp vec3 above max", "[vector]")
{
    vec3f value = {4.0f, 8.0f, 10.0f};
    vec3f min_val = {1.0f, 3.0f, 5.0f};
    vec3f max_val = {3.0f, 7.0f, 9.0f};
    vec3f result = {3.0f, 7.0f, 9.0f};

    CHECK(equal(vector::clamp(value, min_val, max_val), result));
}

// ==================== SIMD Helper Tests ====================

TEST_CASE("vector::replicate", "[vector]")
{
    vec4f result = {5.0f, 5.0f, 5.0f, 5.0f};

    CHECK(equal(vector::replicate(5.0f), result));
}

TEST_CASE("vector::set", "[vector]")
{
    vec4f result = {1.0f, 2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::set(1.0f, 2.0f, 3.0f, 4.0f), result));
}

TEST_CASE("vector::set partial", "[vector]")
{
    vec4f result = {1.0f, 2.0f, 0.0f, 0.0f};

    CHECK(equal(vector::set(1.0f, 2.0f), result));
}

TEST_CASE("vector::get_x", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::get_x(math::load(a)), 1.0f));
}

TEST_CASE("vector::get_y", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::get_y(math::load(a)), 2.0f));
}

TEST_CASE("vector::get_z", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::get_z(math::load(a)), 3.0f));
}

TEST_CASE("vector::get_w", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};

    CHECK(equal(vector::get_w(math::load(a)), 4.0f));
}

// ==================== Operator Tests ====================

TEST_CASE("vector operator+ vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {5.0f, 6.0f, 7.0f, 8.0f};
    vec4f result = {6.0f, 8.0f, 10.0f, 12.0f};

    CHECK(equal(a + b, result));
}

TEST_CASE("vector operator+= vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {5.0f, 6.0f, 7.0f, 8.0f};
    vec4f result = {6.0f, 8.0f, 10.0f, 12.0f};

    a += b;
    CHECK(equal(a, result));
}

TEST_CASE("vector operator- vec4", "[vector]")
{
    vec4f a = {5.0f, 6.0f, 7.0f, 8.0f};
    vec4f b = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f result = {4.0f, 4.0f, 4.0f, 4.0f};

    CHECK(equal(a - b, result));
}

TEST_CASE("vector operator-= vec4", "[vector]")
{
    vec4f a = {5.0f, 6.0f, 7.0f, 8.0f};
    vec4f b = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f result = {4.0f, 4.0f, 4.0f, 4.0f};

    a -= b;
    CHECK(equal(a, result));
}

TEST_CASE("vector operator* vec4 vec4", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f b = {2.0f, 3.0f, 4.0f, 5.0f};
    vec4f result = {2.0f, 6.0f, 12.0f, 20.0f};

    CHECK(equal(a * b, result));
}

TEST_CASE("vector operator* vec4 scalar", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f result = {2.0f, 4.0f, 6.0f, 8.0f};

    CHECK(equal(a * 2.0f, result));
    CHECK(equal(2.0f * a, result));
}

TEST_CASE("vector operator*= vec4 scalar", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 4.0f};
    vec4f result = {2.0f, 4.0f, 6.0f, 8.0f};

    a *= 2.0f;
    CHECK(equal(a, result));
}

TEST_CASE("vector operator/ vec4 vec4", "[vector]")
{
    vec4f a = {4.0f, 6.0f, 8.0f, 10.0f};
    vec4f b = {2.0f, 2.0f, 2.0f, 2.0f};
    vec4f result = {2.0f, 3.0f, 4.0f, 5.0f};

    CHECK(equal(a / b, result));
}

TEST_CASE("vector operator/ vec4 scalar", "[vector]")
{
    vec4f a = {2.0f, 4.0f, 6.0f, 8.0f};
    vec4f result = {1.0f, 2.0f, 3.0f, 4.0f};

    CHECK(equal(a / 2.0f, result));
}

TEST_CASE("vector operator/= vec4 scalar", "[vector]")
{
    vec4f a = {2.0f, 4.0f, 6.0f, 8.0f};
    vec4f result = {1.0f, 2.0f, 3.0f, 4.0f};

    a /= 2.0f;
    CHECK(equal(a, result));
}

// ==================== Normalized Vector Length Test ====================

TEST_CASE("vector::normalize length equals 1", "[vector]")
{
    vec4f a = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f normalized = vector::normalize(a);

    CHECK(equal(vector::length(normalized), 1.0f));
}

TEST_CASE("vector::cross perpendicular vectors", "[vector]")
{
    vec3f a = {1.0f, 0.0f, 0.0f};
    vec3f b = {0.0f, 1.0f, 0.0f};
    vec3f result = vector::cross(a, b);

    CHECK(equal(vector::dot(result, a), 0.0f));
    CHECK(equal(vector::dot(result, b), 0.0f));
}
} // namespace violet::test