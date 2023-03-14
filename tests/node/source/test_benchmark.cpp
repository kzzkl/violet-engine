#include "test_common.hpp"
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
    core::world world;

    timer.start();
    for (std::size_t i = 0; i < 1000000; i++)
        static_cast<void>(world.create());

    auto elapsed = timer.elapse();
    std::cout << "Create entities: " << elapsed << " seconds" << std::endl;
}

TEST_CASE("Iterating entities", "[benchmark]")
{
    timer timer;
    core::world world;

    for (std::size_t i = 0; i < 1000000; ++i)
    {
        const auto entity = world.create();
        world.add<position>(entity);
    }

    timer.start();

    world.each<core::view<position>>([](position& position) { position.x = 100; });

    auto elapsed = timer.elapse();
    std::cout << "Iterating entities without entity: " << elapsed << " seconds" << std::endl;

    timer.start();
    world.each<core::view<position>>([](position& position) { position.x = 100; });
    elapsed = timer.elapse();
    std::cout << "Iterating entities with entity: " << elapsed << " seconds" << std::endl;
}

TEST_CASE("Access components", "[benchmark]")
{
    timer timer;
    core::world world;

    core::node n1("test_node", &world);
    n1.add_component<position>();

    core::component_ptr<position> p = n1.get_component<position>();

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