#include "ecs/component.hpp"
#include "test_common.hpp"

using namespace ash::ecs;

namespace ash::test
{
TEST_CASE("get type id", "[component_index]")
{
    component_id type0 = component_index::value<char>();
    component_id type1 = component_index::value<int>();
    component_id type2 = component_index::value<position>();
    component_id type3 = component_index::value<long>();
    component_id type4 = component_index::value<int>();

    CHECK(type1 != type2);
    CHECK(type2 != type3);
    CHECK(type1 != type3);
    CHECK(type1 == type4);
}
} // namespace ash::test