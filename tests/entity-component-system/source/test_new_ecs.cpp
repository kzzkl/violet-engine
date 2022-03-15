#include "test_common.hpp"
#include "world.hpp"

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

    entity e = w.create();

    w.add<int>(e);

    int& i = w.get_component<int>(e);
    i = 99;

    auto& h = w.get_component<hierarchies>(e);
    h.children.push_back(1);
    h.children.push_back(2);
    h.children.push_back(3);

    void* hp = &h;

    w.add<char>(e);
    w.get_component<char>(e) = 9;

    auto& h2 = w.get_component<hierarchies>(e);
    void* h2p = &h2;

    char c = w.get_component<char>(e);
    int ii = w.get_component<int>(e);

    w.remove<int, char>(e);

    auto& h3 = w.get_component<hierarchies>(e);
    void* h3p = &h3;
}