#include "test_common.hpp"
#include "world.hpp"
#include <chrono>
#include <iostream>

using namespace ash::ecs;

namespace ash::test
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
        static_cast<void>(world.create());

    auto elapsed = timer.elapse();
    std::cout << "Create entities: " << elapsed << " seconds" << std::endl;
}

TEST_CASE("Iterating entities", "[benchmark]")
{
    timer timer;
    world world;
    world.register_component<position>();

    for (std::size_t i = 0; i < 1000000; ++i)
    {
        const auto entity = world.create();
        world.add<position>(entity);
    }

    timer.start();
    auto view = world.make_view<position>();
    view->each([](entity entity, position& position) { position.x = 100; });

    auto elapsed = timer.elapse();
    std::cout << "Iterating entities without entity: " << elapsed << " seconds" << std::endl;

    timer.start();
    view->each([](entity entity, position& position) { position.x = 100; });
    elapsed = timer.elapse();
    std::cout << "Iterating entities with entity: " << elapsed << " seconds" << std::endl;
}
} // namespace ash::test