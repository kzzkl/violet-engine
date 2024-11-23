#include "test_common.hpp"
#include <chrono>
#include <iostream>

namespace violet::test
{
class timer
{
public:
    void start() noexcept
    {
        m_start = std::chrono::steady_clock::now();
    }

    double elapse() const noexcept
    {
        auto duration = std::chrono::steady_clock::now() - m_start;
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() * 0.001;
    }

private:
    std::chrono::steady_clock::time_point m_start;
};

TEST_CASE("Create entities", "[benchmark]")
{
    return;

    static constexpr std::size_t entity_count = 10000000;

    timer timer;

    world world;
    world.register_component<position>();
    world.register_component<rotation>();
    world.register_component<mesh>();

    std::vector<entity> entities;
    entities.reserve(entity_count);

    timer.start();
    for (std::size_t i = 0; i < entity_count; ++i)
    {
        entities.push_back(world.create());
    }
    std::cout << "Create entities: " << timer.elapse() << "s" << std::endl;

    timer.start();
    for (std::size_t i = 0; i < entity_count; ++i)
    {
        world.add_component<mesh>(entities[i]);

        if (i % 2 == 0)
        {
            world.add_component<position>(entities[i]);
        }

        if (i % 3 == 0)
        {
            world.add_component<rotation>(entities[i]);
        }
    }
    std::cout << "Add components: " << timer.elapse() << "s" << std::endl;

    timer.start();
    world.get_view().write<mesh>().each([](mesh& mesh) {});
    std::cout << "Write components: " << timer.elapse() << "s" << std::endl;

    struct render_instance
    {
        geometry* geometry;
        std::uint32_t vertex_offset;
        std::uint32_t vertex_count;
        std::uint32_t index_offset;
        std::uint32_t index_count;

        material* material;
    };

    std::vector<render_instance> render_instances;
    std::unordered_map<material*, std::size_t> material_instance_count;

    for (std::size_t i = 0; i < 100; ++i)
    {
        timer.start();
        world.get_view().read<mesh>().each(
            [&render_instances](const mesh& mesh)
            {
                for (material* material : mesh.materials)
                {
                    render_instance instance = {
                        .geometry = mesh.geometry,
                        .vertex_offset = mesh.vertex_offset,
                        .vertex_count = mesh.vertex_count,
                        .index_offset = mesh.index_offset,
                        .index_count = mesh.index_count,
                        .material = material};
                    render_instances.push_back(instance);
                }
            });
        render_instances.clear();
        std::cout << "Frame " << i << ": " << timer.elapse() << "s" << std::endl;
    }
}

TEST_CASE("Iterating entities", "[benchmark]")
{
    timer timer;
    world world;
}

TEST_CASE("Access components", "[benchmark]")
{
    timer timer;
    world world;
}
} // namespace violet::test