#include "test_common.hpp"
#include "ecs/world.hpp"

using namespace ash::ecs;

namespace ash::test
{
TEST_CASE("world::create", "[world]")
{
    world world;
    entity e1 = world.create();
    CHECK(e1.index == 0);
    CHECK(e1.version == 0);

    entity e2 = world.create();
    CHECK(e2.index == 1);
    CHECK(e2.version == 0);

    world.release(e2);

    entity e3 = world.create();
    CHECK(e3.index == 1);
    CHECK(e3.version == 1);
}

TEST_CASE("world::add & world::remove", "[world]")
{
    life_counter<0>::reset();

    world world;
    world.register_component<life_counter<0>>();
    world.register_component<life_counter<1>>();

    entity e1 = world.create();
    entity e2 = world.create();

    world.add<life_counter<0>>(e1);
    CHECK(life_counter<0>::check(1, 0, 0, 0, 0, 0));

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
    world world;
    world.register_component<position>();
    world.register_component<rotation>();

    std::vector<entity> entities;

    for (std::size_t i = 0; i < 3; ++i)
    {
        entity e = world.create();
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
    world world;
    world.register_component<position>();
    world.register_component<int>();

    entity e1 = world.create();
    world.add<position>(e1);

    position& p1 = world.component<position>(e1);
    position* ptr1 = &p1;
    p1 = {1, 2, 3};

    world.add<int>(e1);
    position& p2 = world.component<position>(e1);
    position* ptr2 = &p2;

    CHECK(ptr1 != ptr2);
    CHECK(p2.x == 1);
    CHECK(p2.y == 2);
    CHECK(p2.z == 3);
}

TEST_CASE("view", "[world]")
{
    world world;
    world.register_component<int>();

    auto view1 = world.make_view<int>();

    auto e1 = world.create();
    auto e2 = world.create();

    world.add<int>(e1);
    world.add<int>(e2);

    world.component<int>(e1) = 1;
    world.component<int>(e2) = 2;

    view1->each([](int& value) { value += 10; });

    CHECK(world.component<int>(e1) == 11);
    CHECK(world.component<int>(e2) == 12);
}

/*TEST_CASE("view a", "[world]")
{
    world world;
    world.register_component<position>();

    std::vector<entity> entities;
    entities.reserve(1000000);

    for (std::size_t i = 0; i < 1000000; ++i)
    {
        const auto entity = world.create();
        world.add<position>(entity);
        entities.push_back(entity);
    }

    int counter = 0;
    auto view = world.make_view<position>();
    view->each([&counter](position& position) {
        position.x = counter;
        ++counter;
    });

    for (int i = 0; i < 1000000; ++i)
        CHECK(world.component<position>(entities[i]).x == i);
}*/
} // namespace ash::test