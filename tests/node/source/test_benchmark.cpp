#include "test_node_common.hpp"
#include <chrono>
#include <iostream>

namespace violet::test
{
class timer
{
public:
    void start() noexcept { m_start = std::chrono::steady_clock::now(); }
    double elapse() const noexcept
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
    }

private:
    std::chrono::steady_clock::time_point m_start;
};

TEST_CASE("Create entities", "[benchmark]")
{
    timer timer;
    world world;

    timer.start();
    for (std::size_t i = 0; i < 1000000; i++)
        static_cast<void>(world.create(nullptr));

    auto elapsed = timer.elapse();
    std::cout << "Create entities: " << elapsed << " seconds" << std::endl;
}

TEST_CASE("Iterating entities", "[benchmark]")
{
    timer timer;
    world world;

    for (std::size_t i = 0; i < 1000000; ++i)
    {
        const auto entity = world.create(nullptr);
        world.add<position>(entity);
    }

    timer.start();
    view<node*, position> view_with_node(world);
    view_with_node.each([](node* node, position& position) { position.x = 100; });
    auto elapsed = timer.elapse();
    std::cout << "Iterating entities with node pointer: " << elapsed << " seconds" << std::endl;

    timer.start();
    view<position> view_without_node(world);
    view_without_node.each([](position& position) { position.x = 100; });
    elapsed = timer.elapse();
    std::cout << "Iterating entities without node pointer: " << elapsed << " seconds" << std::endl;
}

TEST_CASE("Access components", "[benchmark]")
{
    timer timer;
    world world;

    node n1("test_node", &world);
    n1.add_component<position>();

    component_ptr<position> p = n1.get_component<position>();

    timer.start();
    for (std::size_t i = 0; i < 1000000; ++i)
        p->x = 100;
    auto elapsed = timer.elapse();
    std::cout << "Save component pointer: " << elapsed << " seconds" << std::endl;

    timer.start();
    for (std::size_t i = 0; i < 1000000; ++i)
        n1.get_component<position>()->x = 100;
    elapsed = timer.elapse();
    std::cout << "Do not save component pointer: " << elapsed << " seconds" << std::endl;
}
} // namespace violet::test