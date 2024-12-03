#include "math/matrix.hpp"
#include "test_common.hpp"

namespace violet::test
{
TEST_CASE("matrix::mul", "[matrix]")
{
    mat4f a = {
        {1.0f, 2.0f, 3.0f, 4.0f},
        {5.0f, 6.0f, 7.0f, 8.0f},
        {9.0f, 10.0f, 11.0f, 12.0f},
        {13.0f, 14.0f, 15.0f, 16.0f}};

    mat4f b = {
        {1.0f, 2.0f, 3.1f, 4.2f},
        {5.3f, 6.4f, 7.5f, 8.6f},
        {9.7f, 0.8f, 1.9f, 2.1f},
        {3.2f, 4.3f, 5.4f, 6.5f}};

    mat4f result = {
        {53.5000000f, 34.4000015f, 45.3999977f, 53.7000008f},
        {130.300003f, 88.4000015f, 117.000000f, 139.300003f},
        {207.100006f, 142.400009f, 188.600006f, 224.899994f},
        {283.900024f, 196.399994f, 260.200012f, 310.500000f}};

    {
        mat4f r = matrix::mul(a, b);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::mul(math::load(a), math::load(b));
        CHECK(equal(r, result));
    }

    vec4f v = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f result_v = matrix::mul(v, a);

    CHECK(equal(result_v, vec4f{38.0f, 44.0f, 50.0f, 56.0f}));
}

TEST_CASE("matrix::mul vector", "[matrix]")
{
    mat4f m = {
        {1.0f, 2.0f, 3.0f, 4.0f},
        {5.0f, 6.0f, 7.0f, 8.0f},
        {9.0f, 10.0f, 11.0f, 12.0f},
        {13.0f, 14.0f, 15.0f, 16.0f}};

    vec4f v = {1.0f, 2.0f, 3.0f, 0.0f};
    vec4f result = {38.0f, 44.0f, 50.0f, 56.0f};

    {
        vec4f r = matrix::mul(v, m);
        CHECK(equal(r, result));
    }

    {
        vec4f_simd r = matrix::mul(math::load(v), math::load(m));
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::scale", "[matrix]")
{
    mat4f m = {
        {1.0f, 2.0f, 3.0f, 4.0f},
        {5.0f, 6.0f, 7.0f, 8.0f},
        {9.0f, 10.0f, 11.0f, 12.0f},
        {13.0f, 14.0f, 15.0f, 16.0f}};

    mat4f result = {
        {0.5f, 1.0f, 1.5f, 2.0f},
        {2.5f, 3.0f, 3.5f, 4.0f},
        {4.5f, 5.0f, 5.5f, 6.0f},
        {6.5f, 7.0f, 7.5f, 8.0f}};

    {
        mat4f r = matrix::mul(m, 0.5f);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::mul(math::load(m), 0.5f);
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::transpose", "[matrix]")
{
    mat4f m = {
        {1.0f, 2.0f, 3.0f, 4.0f},
        {5.0f, 6.0f, 7.0f, 8.0f},
        {9.0f, 10.0f, 11.0f, 12.0f},
        {13.0f, 14.0f, 15.0f, 16.0f}};

    mat4f result = {
        {1.0f, 5.0f, 9.0f, 13.0f},
        {2.0f, 6.0f, 10.0f, 14.0f},
        {3.0f, 7.0f, 11.0f, 15.0f},
        {4.0f, 8.0f, 12.0f, 16.0f}};

    {
        mat4f r = matrix::transpose(m);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::transpose(math::load(m));
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::inverse", "[matrix]")
{
    // Matrix inverse.
    mat4f m = {
        {0.222222209f, 0.0838168412f, 0.0f, 12.4122639f},
        {-0.428872883f, 0.666666687f, 0.0f, -101.074692f},
        {0.244465783f, -0.0900633037f, 0.0f, -56.2753143f},
        {6.90355968f, 8.28427219f, 1.0f, 599.5f}};

    mat4f result = {
        {2.91421342f, -0.22496891f, 1.04682934f, 0.0f},
        {3.05325222f, 0.971404493f, -1.07128024f, 0.0f},
        {-50.0724525f, -4.97640181f, 8.54683495f, 1.0f},
        {0.00777320424f, -0.00253192894f, -0.0115077635f, 0.0f}};

    {
        mat4f r = matrix::inverse(m);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::inverse(math::load(m));
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::inverse transform", "[matrix]")
{
    mat4f m = {
        {0.321895123f, 0.202351734f, -0.124246888f, 0.0f},
        {-0.621234417f, 1.60947561f, 1.01175869f, 0.0f},
        {0.354115546f, -0.217432037f, 0.563316464f, 0.0f},
        {10.0f, 20.0f, -5.0f, 1.0f}};

    mat4f result = {
        {2.01184464f, -0.155308619f, 0.72268486f, -0.0f},
        {1.26469851f, 0.402368963f, -0.443738937f, 0.0f},
        {-0.776543081f, 0.252939701f, 1.14962554f, 0.0f},
        {-49.2951355f, -5.22959471f, 7.39605761f, 1.0f}};

    {
        mat4f r = matrix::inverse_transform(m);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::inverse_transform(math::load(m));
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::inverse transform no scale", "[matrix]")
{
    mat4f m = {
        {0.804737806f, 0.505879343f, -0.310617208f, 0.0f},
        {-0.310617208f, 0.804737806f, 0.505879343f, 0.0f},
        {0.505879343f, -0.310617208f, 0.804737806f, 0.0f},
        {10.0f, 20.0f, -5.0f, 1.0f}};

    mat4f result = {
        {0.804737866f, -0.310617208f, 0.505879402f, 0.0f},
        {0.505879402f, 0.804737866f, -0.310617208f, 0.0f},
        {-0.310617208f, 0.505879402f, 0.804737866f, 0.0f},
        {-19.7180519f, -10.4591875f, 5.17723942f, 1.0f}};

    {
        mat4f r = matrix::inverse_transform_no_scale(m);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::inverse_transform_no_scale(math::load(m));
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::rotation_quaternion", "[matrix]")
{
    vec4f quat = {0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f};

    mat4f result = {
        {0.886326671f, 0.401883811f, -0.230031431f, 0.0f},
        {-0.366907388f, 0.912558973f, 0.180596486f, 0.0f},
        {0.282496035f, -0.0756672472f, 0.956279457f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}};

    {
        mat4f r = matrix::rotation_quaternion(quat);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::rotation_quaternion(math::load(quat));
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::affine_transform", "[matrix]")
{
    vec3f scale = {0.5f, 0.2f, 0.3f};
    vec4f rotation = {0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f};
    vec3f translation = {6.0f, 5.0f, 4.5f};

    mat4f result = {
        {0.443163335f, 0.200941905f, -0.115015715f, 0.0f},
        {-0.0733814761f, 0.182511792f, 0.0361192971f, 0.0f},
        {0.0847488120f, -0.0227001756f, 0.286883861f, 0.0f},
        {6.0f, 5.0f, 4.5f, 1.0f}};

    {
        mat4f r = matrix::affine_transform(scale, rotation, translation);
        CHECK(equal(r, result));
    }

    {
        mat4f_simd r = matrix::affine_transform(
            math::load(scale),
            math::load(rotation),
            math::load(translation));
        CHECK(equal(r, result));
    }
}

TEST_CASE("matrix::decompose", "[matrix]")
{
    mat4f m = {
        {0.443163335f, 0.200941905f, -0.115015715f, 0.0f},
        {-0.0733814761f, 0.182511792f, 0.0361192971f, 0.0f},
        {0.0847488120f, -0.0227001756f, 0.286883861f, 0.0f},
        {6.0f, 5.0f, 4.5f, 1.0f}};

    vec3f scale = {0.5f, 0.2f, 0.3f};
    vec4f rotation = {0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f};
    vec3f translation = {6.0f, 5.0f, 4.5f};

    {
        vec3f s;
        vec4f r;
        vec3f t;
        matrix::decompose(m, s, r, t);
        CHECK(equal(s, scale));
        CHECK(equal(r, rotation));
        CHECK(equal(t, translation));
    }

    {
        vec4f_simd s;
        vec4f_simd r;
        vec4f_simd t;

        matrix::decompose(math::load(m), s, r, t);
        CHECK(equal(s, scale));
        CHECK(equal(r, rotation));
        CHECK(equal(t, translation));
    }
}

TEST_CASE("matrix::orthographic", "[matrix]")
{
    // Center.
    {
        mat4f result = {
            {0.1f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.2f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0333333351, 0.0f},
            {0.0f, 0.0f, 0.333333343, 1.0f}};

        mat4f r = matrix::orthographic(20.0f, 10.0f, -10.0f, 20.0f);
        CHECK(equal(r, result));
    }

    // Off center.
    {
        mat4f result = {
            {0.222222224f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.222222224f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.111111112f, 0.0f},
            {0.111111112f, -0.777777791f, 0.333333343f, 1.0f}};

        mat4f r = matrix::orthographic(-5.0f, 4.0f, -1.0f, 8.0f, -3.0f, 6.0f);
        CHECK(equal(r, result));
    }
}
} // namespace violet::test