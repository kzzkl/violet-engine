#include "math/math.hpp"
#include "test_common.hpp"
#include <cmath>

namespace violet::test
{
TEST_CASE("sin_cos", "[misc]")
{
    auto [s, c] = math::sin_cos(0.358f);
    CHECK(equal(s, std::sin(0.358f)));
    CHECK(equal(c, std::cos(0.358f)));
}
} // namespace violet::test