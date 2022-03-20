#include "test_common.hpp"
#include <cmath>

using namespace ash::math;

namespace ash::test
{
namespace math_plain
{
TEST_CASE("make quaternion", "[quaternion]")
{
    // axis angle
    {
        float4 axis = vector::normalize(float4{1.0f, 2.0f, 3.0f, 0.0f});
        float4 quat = quaternion::make_from_axis_angle(axis, 0.25f);
        CHECK(equal(quat, float4{0.0333207250f, 0.0666414499f, 0.0999621749f, 0.992197692f}));
    }

    // euler
    {
        float4 euler = float4{1.0f, 2.0f, 3.0f, 0.0f};
        float4 quat = quaternion::make_from_euler(euler);
        CHECK(equal(quat, float4{0.754933715f, -0.206149280f, 0.444435060f, 0.435952783f}));
    }
}
} // namespace math_plain
} // namespace ash::test