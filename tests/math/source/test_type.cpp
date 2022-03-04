#include "test_common.hpp"

using namespace ash::math;

namespace ash::test
{
TEST_CASE("type trait", "[trait]")
{
    CHECK(is_any_of<int, int, char, long>::value == true);
    CHECK(is_any_of<int, char, char, long>::value == false);

    CHECK(is_packed_1d<int2>::value);
    CHECK(is_packed_1d<int>::value == false);

    CHECK(is_packed_2d<int2x2>::value);
    CHECK(is_packed_2d<int>::value == false);

    CHECK(is_row_size_equal<int2x2, 2>::value == true);
    CHECK(is_row_size_equal<int2x2, 3>::value == false);

    CHECK(is_col_size_equal<int2x2, 2>::value == true);
    CHECK(is_col_size_equal<int2x2, 3>::value == false);
}
} // namespace ash::test