#include "test_common.hpp"

namespace violet::test
{
TEST_CASE("world::create", "[world]")
{
    core::world world;
    core::entity e1 = world.create();
    CHECK(e1.index == 0);
    CHECK(e1.entity_version == 0);

    core::entity e2 = world.create();
    CHECK(e2.index == 1);
    CHECK(e2.entity_version == 0);

    world.release(e2);

    core::entity e3 = world.create();
    CHECK(e3.index == 1);
    CHECK(e3.entity_version == 1);
}

TEST_CASE("world::add & world::remove", "[world]")
{
    life_counter<0>::reset();

    core::world world;

    core::entity e1 = world.create();
    core::entity e2 = world.create();

    world.add<life_counter<0>>(e1);
    CHECK(life_counter<0>::check(1, 0, 0, 0, 0, 0));
    CHECK(e1.index == 0);

    world.add<life_counter<1>>(e1);
    CHECK(life_counter<0>::check(1, 0, 1, 0, 0, 1));
    CHECK(life_counter<1>::check(1, 0, 0, 0, 0, 0));

    world.add<life_counter<0>>(e2);
    CHECK(life_counter<0>::check(2, 0, 1, 0, 0, 1));

    world.remove<life_counter<1>>(e1);
    CHECK(life_counter<0>::check(2, 0, 2, 0, 0, 2));
    CHECK(life_counter<1>::check(1, 0, 0, 0, 0, 1));
}

TEST_CASE("world::add & world::remove 2", "[world]")
{
    core::world world;
    std::vector<core::entity> entities;

    for (std::size_t i = 0; i < 3; ++i)
    {
        core::entity e = world.create();
        world.add<position>(e);
        entities.push_back(e);
    }

    for (std::size_t i = 0; i < 3; ++i)
    {
        if (i % 2 == 0)
            world.add<rotation>(entities[i]);
    }
}

TEST_CASE("world::component", "[world]")
{
    core::world world;

    core::entity e1 = world.create();
    world.add<position>(e1);

    position& p1 = world.get_component<position>(e1);
    position* ptr1 = &p1;
    p1 = {1, 2, 3};

    world.add<int>(e1);
    position& p2 = world.get_component<position>(e1);
    position* ptr2 = &p2;

    CHECK(ptr1 != ptr2);
    CHECK(p2.x == 1);
    CHECK(p2.y == 2);
    CHECK(p2.z == 3);
}

TEST_CASE("view", "[world]")
{
    core::world world;

    core::entity e1 = world.create();
    core::entity e2 = world.create();

    world.add<int>(e1);
    world.add<int>(e2);

    world.get_component<int>(e1) = 1;
    world.get_component<int>(e2) = 2;

    core::view<int> view(world);
    view.each([](int& value) { value += 10; });

    CHECK(world.get_component<int>(e1) == 11);
    CHECK(world.get_component<int>(e2) == 12);
}
} // namespace violet::test