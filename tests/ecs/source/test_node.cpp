#include "test_common.hpp"

namespace violet::test
{
TEST_CASE("component handle", "[node]")
{
    world world;

    node n1("test_node", &world);

    n1.add_component<position>();

    component_ptr<position> handle = n1.get_component<position>();
    handle->x = 10;

    n1.add_component<rotation>();
    CHECK(handle->x == 10);

    n1.remove_component<position>();
    CHECK(!handle.is_valid());
}
} // namespace violet::test