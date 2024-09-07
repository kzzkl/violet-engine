#include "test_common.hpp"

namespace violet::test
{
TEST_CASE("Get Type ID", "[component_index]")
{
    component_id type0 = component_index::value<char>();
    component_id type1 = component_index::value<int>();
    component_id type2 = component_index::value<position>();
    component_id type3 = component_index::value<long>();
    component_id type4 = component_index::value<int>();
    component_id type5 = component_index::value<const int>();

    CHECK(type1 != type2);
    CHECK(type2 != type3);
    CHECK(type1 != type3);
    CHECK(type1 == type4);
    CHECK(type5 == type4);
}

TEST_CASE("world::create", "[world]")
{
    world world;
    entity e1 = world.create();
    CHECK(e1.id == 0);
    CHECK(e1.version == 0);

    entity e2 = world.create();
    CHECK(e2.id == 1);
    CHECK(e2.version == 0);

    world.destroy(e2);

    entity e3 = world.create();
    CHECK(e3.id == 1);
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

    world.add_component<life_counter<0>>(e1);
    CHECK(life_counter<0>::check(1, 0, 0, 0, 0, 0));
    CHECK(e1.id == 0);

    world.add_component<life_counter<1>>(e1);
    CHECK(life_counter<0>::check(1, 0, 1, 0, 0, 1));
    CHECK(life_counter<1>::check(1, 0, 0, 0, 0, 0));

    world.add_component<life_counter<0>>(e2);
    CHECK(life_counter<0>::check(2, 0, 1, 0, 0, 1));

    world.remove_component<life_counter<1>>(e1);
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
        world.add_component<position>(e);
        entities.push_back(e);
    }

    for (std::size_t i = 0; i < 3; ++i)
    {
        if (i % 2 == 0)
            world.add_component<rotation>(entities[i]);
    }
}

TEST_CASE("world::component", "[world]")
{
    world world;
    world.register_component<position>();
    world.register_component<int>();

    entity e1 = world.create();
    world.add_component<position>(e1);

    position& p1 = world.get_component<position>(e1);
    position* ptr1 = &p1;
    p1 = {1, 2, 3};

    world.add_component<int>(e1);
    position& p2 = world.get_component<position>(e1);
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

    entity e1 = world.create();
    entity e2 = world.create();

    world.add_component<int>(e1);
    world.add_component<int>(e2);

    world.get_component<int>(e1) = 1;
    world.get_component<int>(e2) = 2;

    auto view = world.get_view().write<int>();

    view.each([](int& value) { value += 10; });

    CHECK(world.get_component<int>(e1) == 11);
    CHECK(world.get_component<int>(e2) == 12);
}

TEST_CASE("Verify the traversal of a View", "[view]")
{
    world world;
    world.register_component<int>();
    world.register_component<std::string>();

    entity e1 = world.create();
    entity e2 = world.create();

    world.add_component<int>(e1);
    world.add_component<int>(e2);
    world.add_component<std::string>(e2);

    auto v1 = world.get_view().write<int>();
    v1.each([](int& value) { value = 100; });

    CHECK(world.get_component<int>(e1) == 100);
    CHECK(world.get_component<int>(e2) == 100);

    auto v2 = world.get_view().write<int>().read<std::string>();
    v2.each([](int& value, const std::string& string) { value = 200; });

    CHECK(world.get_component<int>(e1) == 100);
    CHECK(world.get_component<int>(e2) == 200);
}

TEST_CASE("Include & Exclude", "[view]")
{
    world world;
    world.register_component<int>();
    world.register_component<std::string>();

    entity e1 = world.create();
    entity e2 = world.create();
    entity e3 = world.create();

    world.add_component<int>(e1);
    world.add_component<int>(e2);
    world.add_component<int>(e3);
    world.add_component<std::string>(e3);

    std::size_t count = 0;

    auto v1 = world.get_view().read<int>();
    v1.each([&count](const int& value) { ++count; });
    CHECK(count == 3);

    auto v2 = world.get_view().read<int>().without<std::string>();
    count = 0;
    v2.each([&count](const int& value) { ++count; });
    CHECK(count == 2);

    auto v3 = world.get_view().without<int>();
    count = 0;
    v3.each([&count]() { ++count; });
    CHECK(count == 0);
}

TEST_CASE("Chunk version", "[world]")
{
    world world;
    world.register_component<int>();
    world.register_component<std::string>();

    entity e1 = world.create();
    entity e2 = world.create();
    entity e3 = world.create();

    world.add_component<int>(e1);
    world.add_component<int>(e2);
    world.add_component<int>(e3);
    world.add_component<std::string>(e3);

    world.add_version();

    std::size_t count = 0;

    /**
     * world version: 2
     * e1: int: 1
     * e2: int: 1
     * e3: int: 1, std::string: 1
     */
    auto v1 = world.get_view().read<int>();
    v1.each(
        [&count](const int& value) { ++count; },
        [](auto& view) { return view.is_updated<int>(0); });
    CHECK(count == 3);

    world.add_version();

    /**
     * world version: 3
     * e1: int: 1
     * e2: int: 1
     * e3: int: 1, std::string: 1
     */
    auto v2 = world.get_view().write<int>().read<std::string>();
    v2.each([](int& value, const std::string& str) { value = 100; });

    /**
     * world version: 3
     * e1: int: 1
     * e2: int: 1
     * e3: int: 3, std::string: 1
     */
    auto v3 = world.get_view().read<std::string>();
    count = 0;
    v3.each(
        [&count](const std::string& str) { ++count; },
        [](auto& view) { return view.is_updated<std::string>(2); });
    CHECK(count == 0);

    /**
     * world version: 3
     * e1: int: 1
     * e2: int: 1
     * e3: int: 3, std::string: 1
     */
    auto v4 = world.get_view().read<int>().read<std::string>();
    count = 0;
    v4.each(
        [&count](const int& value, const std::string& str) { ++count; },
        [](auto& view) { return view.is_updated<int>(2); });
    CHECK(count == 1);

    /**
     * world version: 3
     * e1: int: 1
     * e2: int: 1
     * e3: int: 3, std::string: 1
     */
    auto v5 = world.get_view().read<int>();
    count = 0;
    v5.each(
        [&count](const int& value) { ++count; },
        [](auto& view) { return view.is_updated<int>(3); });
    CHECK(count == 0);

    /**
     * world version: 3
     * e1: int: 1
     * e2: int: 1
     * e3: int: 3, std::string: 1
     */
    auto v6 = world.get_view().read<int>();
    count = 0;
    v6.each(
        [&count](const int& value) { ++count; },
        [](auto& view) { return view.is_updated<int>(1); });
    CHECK(count == 1);

    world.add_version();
    world.get_component<int>(e1) = 100;

    /**
     * world version: 4
     * e1: int: 4
     * e2: int: 4
     * e3: int: 3, std::string: 1
     */
    auto v7 = world.get_view().read<int>();
    count = 0;
    v7.each(
        [&count](const int& value) { ++count; },
        [](auto& view) { return view.is_updated<int>(3); });
    CHECK(count == 2);

    /**
     * world version: 4
     * e1: int: 4
     * e2: int: 4
     * e3: int: 3, std::string: 1
     */
    auto v8 = world.get_view().read<int>().read<std::string>();
    count = 0;
    v8.each(
        [&count](const int& value, const std::string& str) { ++count; },
        [](auto& view) { return view.is_updated<int>(2) && !view.is_updated<std::string>(2); });
    CHECK(count == 1);
}
} // namespace violet::test