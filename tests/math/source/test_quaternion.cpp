#include "test_common.hpp"
#include <cmath>

namespace violet::test
{
TEST_CASE("quaternion::rotation_axis", "[quaternion]")
{
    vec3f axis = vector::normalize(vec3f{1.0f, 2.0f, 3.0f});
    vec4f result = {0.033320725f, 0.0666414499f, 0.0999621749f, 0.992197692f};

    {
        vec4f quat = quaternion::from_axis_angle(axis, 0.25f);
        CHECK(equal(quat, result));
    }

    {
        vec4f_simd quat = quaternion::from_axis_angle(math::load(axis), 0.25f);
        CHECK(equal(quat, result));
    }
}

TEST_CASE("quaternion::rotation_euler", "[quaternion]")
{
    vec3f euler = {0.2f, 0.3f, 0.4f};
    vec4f result = {0.126285180f, 0.126116529f, 0.180835575f, 0.967184186f};

    {
        vec4f quat = quaternion::from_euler(euler);
        CHECK(equal(quat, result));
    }

    {
        vec4f_simd quat = quaternion::from_euler(math::load(euler));
        CHECK(equal(quat, result));
    }
}

TEST_CASE("quaternion::rotation_matrix", "[quaternion]")
{
    mat4f m = {
        {0.886326671f, 0.401883781f, -0.230031431f, 0.0f},
        {-0.366907358f, 0.912558973f, 0.180596486f, 0.0f},
        {0.282496035f, -0.0756672472f, 0.956279457f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}};
    vec4f result = {0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f};

    {
        vec4f quat = quaternion::from_matrix(m);
        CHECK(equal(quat, result));
    }

    {
        vec4f_simd quat = quaternion::from_matrix(math::load(m));
        CHECK(equal(quat, result));
    }
}

TEST_CASE("quaternion::mul", "[quaternion]")
{
    vec4f quat1 = {0.033320725f, 0.0666414499f, 0.0999621749f, 0.992197692f};
    vec4f quat2 = {0.126285180f, 0.126116529f, 0.180835575f, 0.967184186f};
    vec4f result = {0.156971410f, 0.196185246f, 0.271892935f, 0.928948700f};

    {
        vec4f quat3 = quaternion::mul(quat1, quat2);
        CHECK(equal(quat3, result));
    }

    {
        vec4f_simd quat3 = quaternion::mul(math::load(quat1), math::load(quat2));
        CHECK(equal(quat3, result));
    }
}

TEST_CASE("quaternion::mul_vec", "[quaternion]")
{
    vec4f quat = {0.033320725f, 0.0666414499f, 0.0999621749f, 0.992197692f};
    vec4f v = {0.126285180f, 0.126116529f, 0.180835575f, 0.0f};
    vec4f result = {0.123301663f, 0.139379591f, 0.172988042f, 0.0f};

    {
        vec4f r = quaternion::mul_vec(quat, v);
        CHECK(equal(r, result));
    }

    {
        vec4f_simd r = quaternion::mul_vec(math::load(quat), math::load(v));
        CHECK(equal(r, result));
    }
}
} // namespace violet::test