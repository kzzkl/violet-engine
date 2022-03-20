#include "test_common.hpp"

using namespace ash::math;

namespace ash::test
{
namespace math_plain
{
TEST_CASE("matrix mul", "[matrix]")
{
    float4x4 a = {
        float4{1.0f,  2.0f,  3.0f,  4.0f },
        float4{5.0f,  6.0f,  7.0f,  8.0f },
        float4{9.0f,  10.0f, 11.0f, 12.0f},
        float4{13.0f, 14.0f, 15.0f, 16.0f}
    };
    float4x4 b = {
        float4{0.1f, 0.2f, 0.3f, 0.4f},
        float4{0.5f, 0.6f, 0.7f, 0.8f},
        float4{0.9f, 1.0f, 1.1f, 1.2f},
        float4{1.3f, 1.4f, 1.5f, 1.6f}
    };

    float4x4 result = matrix::mul(a, b);
    CHECK(equal(
        result,
        float4x4{
            float4{9.0f,  10.0f, 11.0f, 12.0f},
            float4{20.2f, 22.8f, 25.4f, 28.0f},
            float4{31.4f, 35.6f, 39.8f, 44.0f},
            float4{42.6f, 48.4f, 54.2f, 60.0f}
    }));
}

TEST_CASE("matrix scale", "[matrix]")
{
    float4x4 a = {
        float4{1.0f,  2.0f,  3.0f,  4.0f },
        float4{5.0f,  6.0f,  7.0f,  8.0f },
        float4{9.0f,  10.0f, 11.0f, 12.0f},
        float4{13.0f, 14.0f, 15.0f, 16.0f}
    };

    CHECK(equal(
        matrix::scale(a, 0.5f),
        float4x4{
            float4{0.5f, 1.0f, 1.5f, 2.0f},
            float4{2.5f, 3.0f, 3.5f, 4.0f},
            float4{4.5f, 5.0f, 5.5f, 6.0f},
            float4{6.5f, 7.0f, 7.5f, 8.0f}
    }));
}

TEST_CASE("matrix transpose", "[matrix]")
{
    float4x4 a = {
        float4{1.0f,  2.0f,  3.0f,  4.0f },
        float4{5.0f,  6.0f,  7.0f,  8.0f },
        float4{9.0f,  10.0f, 11.0f, 12.0f},
        float4{13.0f, 14.0f, 15.0f, 16.0f}
    };
    float4x4 result = matrix::transpose(a);

    CHECK(equal(
        result,
        float4x4{
            float4{1.0f, 5.0f, 9.0f,  13.0f},
            float4{2.0f, 6.0f, 10.0f, 14.0f},
            float4{3.0f, 7.0f, 11.0f, 15.0f},
            float4{4.0f, 8.0f, 12.0f, 16.0f}
    }));
}

TEST_CASE("matrix determinant", "[matrix]")
{
    float4x4 m4x4 = {
        float4{1.0f, 7.0f, 8.0f, 5.0f},
        float4{6.0f, 5.0f, 4.0f, 4.0f},
        float4{5.0f, 4.0f, 2.0f, 8.0f},
        float4{1.0f, 6.0f, 4.0f, 9.0f}
    };
    CHECK(equal(matrix::determinant(m4x4), -300.0f));
}

TEST_CASE("matrix identity", "[matrix]")
{
    float4x4 result = matrix::identity();
    CHECK(equal(
        result,
        float4x4{
            float4{1.0f, 0.0f, 0.0f, 0.0f},
            float4{0.0f, 1.0f, 0.0f, 0.0f},
            float4{0.0f, 0.0f, 1.0f, 0.0f},
            float4{0.0f, 0.0f, 0.0f, 1.0f}
    }));
}

TEST_CASE("make scaling matrix", "[matrix]")
{
    float4x4 result = matrix::scaling(float4{0.5f, 1.0f, 2.0f, 0.0f});
    CHECK(equal(
        result,
        float4x4{
            float4{0.5f, 0.0f, 0.0f, 0.0f},
            float4{0.0f, 1.0f, 0.0f, 0.0f},
            float4{0.0f, 0.0f, 2.0f, 0.0f},
            float4{0.0f, 0.0f, 0.0f, 1.0f}
    }));
}

TEST_CASE("make axis rotation matrix", "[matrix]")
{
    float4 axis = vector::normalize(float4{1.0f, 2.0f, 3.0f, 0.0f});
    float4x4 result = matrix::rotation_axis(axis, 0.5f);

    CHECK(equal(
        result,
        float4x4{
            float4{0.886326671f,  0.401883781f,   -0.230031431f, 0.0f},
            float4{-0.366907358f, 0.912558973f,   0.180596486f,  0.0f},
            float4{0.282496035f,  -0.0756672472f, 0.956279457f,  0.0f},
            float4{0.0f,          0.0f,           0.0f,          1.0f}
    }));

    result = matrix::rotation_x_axis(0.5f);
    CHECK(equal(
        result,
        float4x4{
            float4{1.0f, 0.0f,          0.0f,         0.0f},
            float4{0.0f, 0.877582550f,  0.479425550f, 0.0f},
            float4{0.0f, -0.479425550f, 0.877582550f, 0.0f},
            float4{0.0f, 0.0f,          0.0f,         1.0f}
    }));

    result = matrix::rotation_y_axis(0.5f);
    CHECK(equal(
        result,
        float4x4{
            float4{0.877582550f, 0.0f, -0.479425550f, 0.0f},
            float4{0.0f,         1.0f, 0.0f,          0.0f},
            float4{0.479425550f, 0.0f, 0.877582550f,  0.0f},
            float4{0.0f,         0.0f, 0.0f,          1.0f}
    }));

    result = matrix::rotation_z_axis(0.5f);
    CHECK(equal(
        result,
        float4x4{
            float4{0.877582550f,  0.479425550f, 0.0f, 0.0f},
            float4{-0.479425550f, 0.877582550f, 0.0f, 0.0f},
            float4{0.0f,          0.0f,         1.0f, 0.0f},
            float4{0.0f,          0.0f,         0.0f, 1.0f}
    }));
}
} // namespace math_plain

namespace math_simd
{
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
        float4x4{
            float4{53.5000000f, 34.4000015f, 45.3999977f, 53.7000008f},
            float4{130.300003f, 88.4000015f, 117.000000f, 139.300003f},
            float4{207.100006f, 142.400009f, 188.600006f, 224.899994f},
            float4{283.900024f, 196.399994f, 260.200012f, 310.500000f}
    }));
}

TEST_CASE("simd matrix scale", "[matrix][simd]")
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
        float4x4{
            float4{0.5f, 1.0f, 1.5f, 2.0f},
            float4{2.5f, 3.0f, 3.5f, 4.0f},
            float4{4.5f, 5.0f, 5.5f, 6.0f},
            float4{6.5f, 7.0f, 7.5f, 8.0f}
    }));
}

TEST_CASE("simd matrix transpose", "[matrix][simd]")
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
        float4x4{
            float4{1.0f, 5.0f, 9.0f,  13.0f},
            float4{2.0f, 6.0f, 10.0f, 14.0f},
            float4{3.0f, 7.0f, 11.0f, 15.0f},
            float4{4.0f, 8.0f, 12.0f, 16.0f}
    }));
}

TEST_CASE("simd matrix identity", "[matrix][simd]")
{
    float4x4_simd a = matrix::identity();

    float4x4 result;
    simd::store(matrix::transpose(a), result);

    CHECK(equal(
        result,
        float4x4{
            float4{1.0f, 0.0f, 0.0f, 0.0f},
            float4{0.0f, 1.0f, 0.0f, 0.0f},
            float4{0.0f, 0.0f, 1.0f, 0.0f},
            float4{0.0f, 0.0f, 0.0f, 1.0f}
    }));
}
} // namespace math_simd
} // namespace ash::test