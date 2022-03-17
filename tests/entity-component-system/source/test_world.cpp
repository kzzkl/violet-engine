#include "test_common.hpp"
#include "world.hpp"

using namespace test;
using namespace ash::ecs;
/*
TEST_CASE("create entity", "[world]")
{
    world w;
    auto entity1 = w.create<int, char, std::string>();
    entity1.get_component<int>() = 99;

    auto handle = w.get_component<int>(entity1.get_entity());
    CHECK(handle.get_component<int>() == 99);
}

TEST_CASE("insert component", "[world]")
{
    world w;
    auto entity = w.create<int, char, std::string>();

    w.insert<test_class>(entity.get_entity());
}

TEST_CASE("erase component", "[world]")
{
    world w;
    auto entity = w.create<int, char, std::string>();

    w.erase<int>(entity.get_entity());
}

TEST_CASE("view", "[world]")
{
    world world;

    auto entity1 = world.create<int, char, test_class>();
    auto entity2 = world.create<int>();

    auto view1 = world.get_view<int>();
    view1.each([](entity e, int value) { INFO("view1: " << e << " " << value); });

    auto view2 = world.get_view<int, test_class>();
    view2.each([](entity e, int value, test_class& test) { INFO("view2: " << e << " " << value); });
}

TEST_CASE("view each", "[world]")
{
    world w;

    auto entity1 = w.create<position, velocity>();
    position& p1 = entity1.get_component<position>();
    velocity& v1 = entity1.get_component<velocity>();
    p1 = {1, 2, 3};
    v1 = {2, 2, 2};

    auto entity2 = w.create<position, velocity>();
    position& p2 = entity2.get_component<position>();
    velocity& v2 = entity2.get_component<velocity>();
    p2 = {4, 5, 6};
    v2 = {3, 3, 3};

    auto view = w.get_view<velocity, position>();
    view.each([](entity e, velocity& v, position& p) {
        p.x += v.x;
        p.y += v.y;
        p.z += v.z;
    });

    CHECK(p1.x == 3);
    CHECK(p1.y == 4);
    CHECK(p1.z == 5);

    CHECK(p2.x == 7);
    CHECK(p2.y == 8);
    CHECK(p2.z == 9);
}*/