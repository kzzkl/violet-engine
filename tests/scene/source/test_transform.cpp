#include "core/context/engine.hpp"
#include "test_scene_common.hpp"

namespace violet::test
{
TEST_CASE("transform update", "[transform]")
{
    node n1("node 1");
    n1.add_component<scene::transform>();
    auto n1_transform = n1.get_component<scene::transform>();
    n1_transform->set_position(1, 1, 1);

    node n2("node 2");
    n2.add_component<scene::transform>();
    auto n2_transform = n2.get_component<scene::transform>();
    n2_transform->set_position(2, 1, 1);

    n1.add(&n2);

    engine::get_module<scene::scene>().update_transform();
}
} // namespace violet::test