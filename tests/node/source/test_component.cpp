#include "test_common.hpp"

namespace violet::test
{
TEST_CASE("get type id", "[component_index]")
{
    core::component_id type0 = core::component_index::value<char>();
    core::component_id type1 = core::component_index::value<int>();
    core::component_id type2 = core::component_index::value<position>();
    core::component_id type3 = core::component_index::value<long>();
    core::component_id type4 = core::component_index::value<int>();

    CHECK(type1 != type2);
    CHECK(type2 != type3);
    CHECK(type1 != type3);
    CHECK(type1 == type4);
}
} // namespace violet::test