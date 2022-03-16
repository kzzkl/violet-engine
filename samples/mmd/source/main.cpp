#include "application.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "pmx_loader.hpp"
#include "window.hpp"

using namespace ash::core;
using namespace ash::graphics;
using namespace ash::window;
using namespace ash::ecs;
using namespace ash::sample::mmd;

namespace ash::sample::mmd
{
struct vertex
{
    math::float3 position;
};

class test_module : public submodule
{
public:
    static constexpr ash::uuid id = "bd58a298-9ea4-4f8d-a79c-e57ae694915a";

public:
    test_module() : submodule("test_module") {}

    virtual bool initialize(const ash::dictionary& config) override
    {
        pmx_loader loader;
        if (!loader.load("resource/White.pmx"))
        {
            ash::log::error("Load pmx failed");
            return -1;
        }

        std::vector<vertex> vertices;
        vertices.reserve(loader.get_vertices().size());
        for (const pmx_vertex& v : loader.get_vertices())
        {
            vertices.push_back(vertex{v.position});
        }

        std::vector<uint32_t> indices;
        indices.reserve(loader.get_indices().size());
        for (uint32_t i : loader.get_indices())
        {
            indices.push_back(i);
        }

        ash::ecs::world& world = get_submodule<ash::ecs::world>();
        entity_id entity = world.create();

        world.add<visual, mesh, material>(entity);

        visual& v = world.get_component<visual>(entity);
        v.group = get_submodule<ash::graphics::graphics>().get_group("");

        mesh& m = world.get_component<mesh>(entity);
        m.vertex_buffer = get_submodule<ash::graphics::graphics>().make_vertex_buffer<vertex>(
            vertices.data(),
            vertices.size());
        m.index_buffer = get_submodule<ash::graphics::graphics>().make_index_buffer<uint32_t>(
            indices.data(),
            indices.size());

        return true;
    }

private:
    std::string m_title;
};
} // namespace ash::sample::mmd

int main()
{
    application app;
    app.install<window>();
    app.install<graphics>();
    app.install<ash::sample::mmd::test_module>();

    app.run();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(600));

    return 0;
}