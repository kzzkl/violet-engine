#include "test_common.hpp"

using namespace violet::math;

namespace violet::test
{
TEST_CASE("matrix::mul", "[matrix]")
{
    math::float4x4 a = {
        math::float4{1.0f,  2.0f,  3.0f,  4.0f },
        math::float4{5.0f,  6.0f,  7.0f,  8.0f },
        math::float4{9.0f,  10.0f, 11.0f, 12.0f},
        math::float4{13.0f, 14.0f, 15.0f, 16.0f}
    };
    math::float4x4 b = {
        math::float4{0.1f, 0.2f, 0.3f, 0.4f},
        math::float4{0.5f, 0.6f, 0.7f, 0.8f},
        math::float4{0.9f, 1.0f, 1.1f, 1.2f},
        math::float4{1.3f, 1.4f, 1.5f, 1.6f}
    };

    math::float4x4 result = matrix::mul(a, b);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{9.0f,  10.0f, 11.0f, 12.0f},
            math::float4{20.2f, 22.8f, 25.4f, 28.0f},
            math::float4{31.4f, 35.6f, 39.8f, 44.0f},
            math::float4{42.6f, 48.4f, 54.2f, 60.0f}
    }));

    float4 v = {1.0f, 2.0f, 3.0f, 0.0f};
    CHECK(equal(matrix::mul(v, a), float4{38.0f, 44.0f, 50.0f, 56.0f}));

    CHECK(equal(
        matrix::mul(a, 0.5f),
        math::float4x4{
            math::float4{0.5f, 1.0f, 1.5f, 2.0f},
            math::float4{2.5f, 3.0f, 3.5f, 4.0f},
            math::float4{4.5f, 5.0f, 5.5f, 6.0f},
            math::float4{6.5f, 7.0f, 7.5f, 8.0f}
    }));
}

TEST_CASE("matrix::transpose", "[matrix]")
{
    math::float4x4 a = {
        math::float4{1.0f,  2.0f,  3.0f,  4.0f },
        math::float4{5.0f,  6.0f,  7.0f,  8.0f },
        math::float4{9.0f,  10.0f, 11.0f, 12.0f},
        math::float4{13.0f, 14.0f, 15.0f, 16.0f}
    };
    math::float4x4 result = matrix::transpose(a);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{1.0f, 5.0f, 9.0f,  13.0f},
            math::float4{2.0f, 6.0f, 10.0f, 14.0f},
            math::float4{3.0f, 7.0f, 11.0f, 15.0f},
            math::float4{4.0f, 8.0f, 12.0f, 16.0f}
    }));
}

TEST_CASE("matrix::determinant", "[matrix]")
{
    math::float4x4 m4x4 = {
        math::float4{1.0f, 7.0f, 8.0f, 5.0f},
        math::float4{6.0f, 5.0f, 4.0f, 4.0f},
        math::float4{5.0f, 4.0f, 2.0f, 8.0f},
        math::float4{1.0f, 6.0f, 4.0f, 9.0f}
    };
    CHECK(equal(matrix::determinant(m4x4), -300.0f));
}

TEST_CASE("matrix::inverse", "[matrix]")
{
    math::float4x4 result;

    // General Matrix inverse.
    math::float4x4 m = {
        math::float4{0.222222209f,  0.0838168412f,  0.0f, 12.4122639f },
        math::float4{-0.428872883f, 0.666666687f,   0.0f, -101.074692f},
        math::float4{0.244465783f,  -0.0900633037f, 0.0f, -56.2753143f},
        math::float4{6.90355968f,   8.28427219f,    1.0f, 599.5f      }
    };

    result = matrix::inverse(m);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{2.91421342f,    -0.22496891f,    1.04682934f,    0.0f},
            math::float4{3.05325222f,    0.971404493f,    -1.07128024f,   0.0f},
            math::float4{-50.0724525f,   -4.97640181f,    8.54683495f,    1.0f},
            math::float4{0.00777320424f, -0.00253192894f, -0.0115077635f, 0.0f}
    }));

    // Transform Matrix inverse.
    math::float4x4 transform_matrix = {
        math::float4{0.321895123f,  0.202351734f,  -0.124246888f, 0.0f},
        math::float4{-0.621234417f, 1.60947561f,   1.01175869f,   0.0f},
        math::float4{0.354115546f,  -0.217432037f, 0.563316464f,  0.0f},
        math::float4{10.0f,         20.0f,         -5.0f,         1.0f}
    };

    result = matrix::inverse_transform(transform_matrix);
    CHECK(equal(
        matrix::inverse_transform(transform_matrix),
        math::float4x4{
            math::float4{2.01184464f,   -0.155308619f, 0.72268486f,   -0.0f},
            math::float4{1.26469851f,   0.402368963f,  -0.443738937f, 0.0f },
            math::float4{-0.776543081f, 0.252939701f,  1.14962554f,   0.0f },
            math::float4{-49.2951355f,  -5.22959471f,  7.39605761f,   1.0f }
    }));

    // Transform Matrix inverse no scale.
    math::float4x4 transform_matrix_no_scale = {
        math::float4{0.804737806f,  0.505879343f,  -0.310617208f, 0.0f},
        math::float4{-0.310617208f, 0.804737806f,  0.505879343f,  0.0f},
        math::float4{0.505879343f,  -0.310617208f, 0.804737806f,  0.0f},
        math::float4{10.0f,         20.0f,         -5.0f,         1.0f}
    };

    result = matrix::inverse_transform_no_scale(transform_matrix_no_scale);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.804737866f,  -0.310617208f, 0.505879402f,  0.0f},
            math::float4{0.505879402f,  0.804737866f,  -0.310617208f, 0.0f},
            math::float4{-0.310617208f, 0.505879402f,  0.804737866f,  0.0f},
            math::float4{-19.7180519f,  -10.4591875f,  5.17723942f,   1.0f}
    }));
}

TEST_CASE("matrix::identity", "[matrix]")
{
    math::float4x4 result = math::matrix::identity();
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{1.0f, 0.0f, 0.0f, 0.0f},
            math::float4{0.0f, 1.0f, 0.0f, 0.0f},
            math::float4{0.0f, 0.0f, 1.0f, 0.0f},
            math::float4{0.0f, 0.0f, 0.0f, 1.0f}
    }));
}

TEST_CASE("matrix::scale", "[matrix]")
{
    math::float4x4 result = math::matrix::scale(float3{0.5f, 1.0f, 2.0f});
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.5f, 0.0f, 0.0f, 0.0f},
            math::float4{0.0f, 1.0f, 0.0f, 0.0f},
            math::float4{0.0f, 0.0f, 2.0f, 0.0f},
            math::float4{0.0f, 0.0f, 0.0f, 1.0f}
    }));
}

TEST_CASE("matrix::rotation_axis", "[matrix]")
{
    math::float4 axis = math::vector::normalize(float4{1.0f, 2.0f, 3.0f, 0.0f});
    math::float4x4 result = math::matrix::rotation_axis(axis, 0.5f);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.886326671f,  0.401883781f,   -0.230031431f, 0.0f},
            math::float4{-0.366907358f, 0.912558973f,   0.180596486f,  0.0f},
            math::float4{0.282496035f,  -0.0756672472f, 0.956279457f,  0.0f},
            math::float4{0.0f,          0.0f,           0.0f,          1.0f}
    }));

    result = math::matrix::rotation_x_axis(0.5f);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{1.0f, 0.0f,          0.0f,         0.0f},
            math::float4{0.0f, 0.877582550f,  0.479425550f, 0.0f},
            math::float4{0.0f, -0.479425550f, 0.877582550f, 0.0f},
            math::float4{0.0f, 0.0f,          0.0f,         1.0f}
    }));

    result = math::matrix::rotation_y_axis(0.5f);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.877582550f, 0.0f, -0.479425550f, 0.0f},
            math::float4{0.0f,         1.0f, 0.0f,          0.0f},
            math::float4{0.479425550f, 0.0f, 0.877582550f,  0.0f},
            math::float4{0.0f,         0.0f, 0.0f,          1.0f}
    }));

    result = math::matrix::rotation_z_axis(0.5f);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.877582550f,  0.479425550f, 0.0f, 0.0f},
            math::float4{-0.479425550f, 0.877582550f, 0.0f, 0.0f},
            math::float4{0.0f,          0.0f,         1.0f, 0.0f},
            math::float4{0.0f,          0.0f,         0.0f, 1.0f}
    }));
}

TEST_CASE("matrix::rotation_quaternion", "[matrix]")
{
    math::float4 quat = {0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f};
    math::float4x4 result = math::matrix::rotation_quaternion(quat);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.886326671f,  0.401883811f,   -0.230031431f, 0.0f},
            math::float4{-0.366907388f, 0.912558973f,   0.180596486f,  0.0f},
            math::float4{0.282496035f,  -0.0756672472f, 0.956279457f,  0.0f},
            math::float4{0.0f,          0.0f,           0.0f,          1.0f}
    }));
}

TEST_CASE("matrix::affine_transform", "[matrix]")
{
    math::float4 scale = {0.5f, 0.2f, 0.3f, 0.0f};
    math::float4 rotation = {0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f};
    math::float4 translation = {6.0f, 5.0f, 4.5f, 0.0f};

    math::float4x4 result = math::matrix::affine_transform(scale, rotation, translation);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.443163335f,   0.200941905f,   -0.115015715f, 0.0f},
            math::float4{-0.0733814761f, 0.182511792f,   0.0361192971f, 0.0f},
            math::float4{0.0847488120f,  -0.0227001756f, 0.286883861f,  0.0f},
            math::float4{6.0f,           5.0f,           4.5f,          1.0f}
    }));
}

TEST_CASE("matrix::decompose", "[matrix]")
{
    math::float4x4 m = {
        math::float4{0.443163335f,   0.200941905f,   -0.115015715f, 0.0f},
        math::float4{-0.0733814761f, 0.182511792f,   0.0361192971f, 0.0f},
        math::float4{0.0847488120f,  -0.0227001756f, 0.286883861f,  0.0f},
        math::float4{6.0f,           5.0f,           4.5f,          1.0f}
    };

    math::float4 scale = {}, rotation = {}, translation = {};
    math::matrix::decompose(m, scale, rotation, translation);
    CHECK(equal(scale, math::float4{0.5f, 0.2f, 0.3f, 0.0f}));
    CHECK(equal(rotation, math::float4{0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f}));
    CHECK(equal(translation, math::float4{6.0f, 5.0f, 4.5f, 1.0f}));
}

TEST_CASE("matrix::orthographic", "[matrix]")
{
    // Center.
    math::float4x4 result = math::matrix::orthographic(-5.0f, 4.0f, 8.0f, -1.0f);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{-0.4f, 0.0f, 0.0f,          0.0f},
            math::float4{0.0f,  0.5f, 0.0f,          0.0f},
            math::float4{0.0f,  0.0f, -0.111111112f, 0.0f},
            math::float4{0.0f,  0.0f, 0.888888896f,  1.0f}
    }));

    // Off center.
    result = math::matrix::orthographic(-5.0f, 4.0f, -1.0f, 8.0f, -3.0f, 6.0f);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.222222224f, 0.0f,          0.0f,         0.0f},
            math::float4{0.0f,         0.222222224f,  0.0f,         0.0f},
            math::float4{0.0f,         0.0f,          0.111111112f, 0.0f},
            math::float4{0.111111112f, -0.777777791f, 0.333333343f, 1.0f}
    }));
}

TEST_CASE("matrix_simd::mul", "[matrix][simd]")
{
    {
        // 2x2
        math::float4_simd a = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);
        math::float4_simd b = math::simd::set(1.0f, 2.0f, 3.0f, 4.0f);

        math::float4 result;
        math::simd::store(math::matrix_simd::mul_mat2(a, b), result);
        CHECK(equal(result, math::float4{7.0f, 10.0f, 15.0f, 22.0f}));
    }

    {
        // 4x4
        math::float4x4_simd a = math::simd::set(
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

        math::float4x4_simd b = math::simd::set(
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

        math::float4x4 result;
        math::simd::store(math::matrix_simd::mul(a, b), result);

        CHECK(equal(
            result,
            math::float4x4{
                math::float4{53.5000000f, 34.4000015f, 45.3999977f, 53.7000008f},
                math::float4{130.300003f, 88.4000015f, 117.000000f, 139.300003f},
                math::float4{207.100006f, 142.400009f, 188.600006f, 224.899994f},
                math::float4{283.900024f, 196.399994f, 260.200012f, 310.500000f}
        }));

        math::float4_simd v = math::simd::set(1.0f, 2.0f, 3.0f, 0.0f);
        math::float4 result_v;
        math::simd::store(math::matrix_simd::mul(v, a), result_v);

        CHECK(equal(result_v, math::float4{38.0f, 44.0f, 50.0f, 56.0f}));
    }
}

TEST_CASE("matrix_simd::scale", "[matrix][simd]")
{
    math::float4x4_simd a = math::simd::set(
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

    math::float4x4 result;
    math::simd::store(math::matrix_simd::scale(a, 0.5f), result);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.5f, 1.0f, 1.5f, 2.0f},
            math::float4{2.5f, 3.0f, 3.5f, 4.0f},
            math::float4{4.5f, 5.0f, 5.5f, 6.0f},
            math::float4{6.5f, 7.0f, 7.5f, 8.0f}
    }));
}

TEST_CASE("matrix_simd::transpose", "[matrix][simd]")
{
    math::float4x4_simd a = math::simd::set(
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

    math::float4x4 result;
    math::simd::store(math::matrix_simd::transpose(a), result);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{1.0f, 5.0f, 9.0f,  13.0f},
            math::float4{2.0f, 6.0f, 10.0f, 14.0f},
            math::float4{3.0f, 7.0f, 11.0f, 15.0f},
            math::float4{4.0f, 8.0f, 12.0f, 16.0f}
    }));
}

TEST_CASE("matrix_simd::inverse", "[matrix][simd]")
{
    math::float4x4 result;

    // Matrix inverse.
    math::float4x4_simd m = math::simd::set(
        0.222222209f,
        0.0838168412f,
        0.0f,
        12.4122639f,
        -0.428872883f,
        0.666666687f,
        0.0f,
        -101.074692f,
        0.244465783f,
        -0.0900633037f,
        0.0f,
        -56.2753143f,
        6.90355968f,
        8.28427219f,
        1.0f,
        599.5f);

    math::simd::store(matrix_simd::inverse(m), result);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{2.91421342f,    -0.22496891f,    1.04682934f,    0.0f},
            math::float4{3.05325222f,    0.971404493f,    -1.07128024f,   0.0f},
            math::float4{-50.0724525f,   -4.97640181f,    8.54683495f,    1.0f},
            math::float4{0.00777320424f, -0.00253192894f, -0.0115077635f, 0.0f}
    }));

    // Transform Matrix inverse.
    math::float4x4_simd transform_matrix = math::simd::set(
        0.321895123f,
        0.202351734f,
        -0.124246888f,
        0.0f,
        -0.621234417f,
        1.60947561f,
        1.01175869f,
        0.0f,
        0.354115546f,
        -0.217432037f,
        0.563316464f,
        0.0f,
        10.0f,
        20.0f,
        -5.0f,
        1.0f);

    math::simd::store(matrix_simd::inverse_transform(transform_matrix), result);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{2.01184464f,   -0.155308619f, 0.72268486f,   -0.0f},
            math::float4{1.26469851f,   0.402368963f,  -0.443738937f, 0.0f },
            math::float4{-0.776543081f, 0.252939701f,  1.14962554f,   0.0f },
            math::float4{-49.2951355f,  -5.22959471f,  7.39605761f,   1.0f }
    }));

    // Transform Matrix inverse no scale.
    math::float4x4_simd transform_matrix_no_scale = math::simd::set(
        0.804737806f,
        0.505879343f,
        -0.310617208f,
        0.0f,
        -0.310617208f,
        0.804737806f,
        0.505879343f,
        0.0f,
        0.505879343f,
        -0.310617208f,
        0.804737806f,
        0.0f,
        10.0f,
        20.0f,
        -5.0f,
        1.0f);

    math::simd::store(matrix_simd::inverse_transform_no_scale(transform_matrix_no_scale), result);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.804737866f,  -0.310617208f, 0.505879402f,  0.0f},
            math::float4{0.505879402f,  0.804737866f,  -0.310617208f, 0.0f},
            math::float4{-0.310617208f, 0.505879402f,  0.804737866f,  0.0f},
            math::float4{-19.7180519f,  -10.4591875f,  5.17723942f,   1.0f}
    }));
}

TEST_CASE("matrix_simd::identity", "[matrix][simd]")
{
    math::float4x4_simd a = math::matrix_simd::identity();

    math::float4x4 result;
    math::simd::store(math::matrix_simd::transpose(a), result);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{1.0f, 0.0f, 0.0f, 0.0f},
            math::float4{0.0f, 1.0f, 0.0f, 0.0f},
            math::float4{0.0f, 0.0f, 1.0f, 0.0f},
            math::float4{0.0f, 0.0f, 0.0f, 1.0f}
    }));
}

TEST_CASE("matrix_simd::rotation_quaternion", "[matrix][simd]")
{
    math::float4_simd quat = simd::set(0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f);
    math::float4x4_simd m = math::matrix_simd::rotation_quaternion(quat);

    math::float4x4 result;
    math::simd::store(math::matrix_simd::rotation_quaternion(quat), result);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.886326671f,  0.401883811f,   -0.230031431f, 0.0f},
            math::float4{-0.366907388f, 0.912558973f,   0.180596486f,  0.0f},
            math::float4{0.282496035f,  -0.0756672472f, 0.956279457f,  0.0f},
            math::float4{0.0f,          0.0f,           0.0f,          1.0f}
    }));
}

TEST_CASE("matrix_simd::affine_transform", "[matrix][simd]")
{
    math::float4_simd scale = math::simd::set(0.5f, 0.2f, 0.3f, 0.0f);
    math::float4_simd rotation =
        math::simd::set(0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f);
    math::float4_simd translation = math::simd::set(6.0f, 5.0f, 4.5f, 0.0f);

    math::float4x4 result;
    math::simd::store(math::matrix_simd::affine_transform(scale, rotation, translation), result);

    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.443163335f,   0.200941905f,   -0.115015715f, 0.0f},
            math::float4{-0.0733814761f, 0.182511792f,   0.0361192971f, 0.0f},
            math::float4{0.0847488120f,  -0.0227001756f, 0.286883861f,  0.0f},
            math::float4{6.0f,           5.0f,           4.5f,          1.0f}
    }));
}

TEST_CASE("matrix_simd::decompose", "[matrix][simd]")
{
    math::float4x4_simd m = math::simd::set(
        0.443163335f,
        0.200941905f,
        -0.115015715f,
        0.0f,
        -0.0733814761f,
        0.182511792f,
        0.0361192971f,
        0.0f,
        0.0847488120f,
        -0.0227001756f,
        0.286883861f,
        0.0f,
        6.0f,
        5.0f,
        4.5f,
        1.0f);

    math::float4_simd scale, rotation, translation;
    math::matrix_simd::decompose(m, scale, rotation, translation);
    CHECK(equal(scale, math::simd::set(0.5f, 0.2f, 0.3f, 0.0f)));
    CHECK(
        equal(rotation, math::simd::set(0.0661214888f, 0.132242978f, 0.198364466f, 0.968912423f)));
    CHECK(equal(translation, math::simd::set(6.0f, 5.0f, 4.5f, 1.0f)));
}

TEST_CASE("matrix_simd::orthographic", "[matrix]")
{
    // Center.
    math::float4x4 result;
    math::simd::store(math::matrix_simd::orthographic(20.0f, 10.0f, -10.0f, 20.0f), result);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.1f, 0.0f, 0.0f,         0.0f},
            math::float4{0.0f, 0.2f, 0.0f,         0.0f},
            math::float4{0.0f, 0.0f, 0.0333333351, 0.0f},
            math::float4{0.0f, 0.0f, 0.333333343,  1.0f}
    }));

    // Off center.
    math::simd::store(
        math::matrix_simd::orthographic(-5.0f, 4.0f, -1.0f, 8.0f, -3.0f, 6.0f),
        result);
    CHECK(equal(
        result,
        math::float4x4{
            math::float4{0.222222224f, 0.0f,          0.0f,         0.0f},
            math::float4{0.0f,         0.222222224f,  0.0f,         0.0f},
            math::float4{0.0f,         0.0f,          0.111111112f, 0.0f},
            math::float4{0.111111112f, -0.777777791f, 0.333333343f, 1.0f}
    }));
}
} // namespace violet::test