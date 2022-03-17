#include "test_common.hpp"
#include "world.hpp"
#include <iostream>

using namespace ash::ecs;

template <>
struct component_trait<int>
{
    static constexpr std::size_t id = ash::uuid("42ec283a-6341-4ae8-8e5e-261c87cb6ef1").hash();
};

template <>
struct component_trait<char>
{
    static constexpr std::size_t id = ash::uuid("47a6b9bc-5d7c-43a8-ac9b-be6cb147dc6c").hash();
};

TEST_CASE("new", "[new]")
{
    world w;
    w.register_component<int>();
    w.register_component<char>();

    auto v = w.create_view<hierarchy, int>();

    entity_id e = w.create();

    w.add<int>(e);

    v->each([](hierarchy& h, int& i) { std::cout << i << std::endl; });

    int& i = w.get_component<int>(e);
    i = 99;

    auto& h = w.get_component<hierarchy>(e);
    h.children.push_back(1);
    h.children.push_back(2);
    h.children.push_back(3);

    void* hp = &h;

    w.add<char>(e);
    w.get_component<char>(e) = 9;

    v->each([](hierarchy& h, int& i) { std::cout << i << std::endl; });

    auto& h2 = w.get_component<hierarchy>(e);
    void* h2p = &h2;

    char c = w.get_component<char>(e);
    int ii = w.get_component<int>(e);

    w.remove<int, char>(e);

    auto& h3 = w.get_component<hierarchy>(e);
    void* h3p = &h3;

    v->each([](hierarchy& h, int& i) { std::cout << i << std::endl; });
}