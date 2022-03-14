#include "test_common.hpp"
#include "world.hpp"

using namespace ash::ecs;

TEST_CASE("new", "[new]")
{
    world w;

    entity e = w.create();

    w.add<int>(e);

    int& i = w.get_component<int>(e);
    i = 99;

    w.add<char>(e);
    w.get_component<char>(e) = 9;

    char c = w.get_component<char>(e);
}