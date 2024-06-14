#include "test_common.hpp"
#include <cmath>

namespace violet::test
{
TEST_CASE("sin_cos", "[misc]")
{
    auto [s, c] = math::sin_cos(0.358f);
    CHECK(equal(s, sin(0.358f)));
    CHECK(equal(c, cos(0.358f)));
}
} // namespace violet::test