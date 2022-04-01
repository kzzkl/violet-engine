#include "test_common.hpp"
#include <cmath>

using namespace ash::math;

namespace ash::test
{
TEST_CASE("sin_cos", "[misc]")
{
    auto [s, c] = sin_cos(0.358f);
    CHECK(equal(s, sin(0.358f)));
    CHECK(equal(c, cos(0.358f)));
}
} // namespace ash::test