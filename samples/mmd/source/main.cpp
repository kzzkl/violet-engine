#include "application.hpp"
#include "graphics.hpp"
#include "log.hpp"
#include "pmx_loader.hpp"
#include "scene.hpp"
#include "window.hpp"

using namespace ash::core;
using namespace ash::graphics;
using namespace ash::window;
using namespace ash::ecs;
using namespace ash::scene;
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
            return false;
        }

        std::vector<vertex> vertices;
        vertices.reserve(loader.get_vertices().size());
        for (const pmx_vertex& v : loader.get_vertices())
        {
            vertices.push_back(vertex{v.position});
        }

        std::vector<std::int32_t> indices;
        indices.reserve(loader.get_indices().size());
        for (std::int32_t i : loader.get_indices())
        {
            indices.push_back(i);
        }

        ash::ecs::world& world = get_submodule<ash::ecs::world>();
        entity_id entity = world.create();

        world.add<visual, mesh, material>(entity);

        visual& v = world.get_component<visual>(entity);
        v.group = get_submodule<ash::graphics::graphics>().get_group("mmd");

        mesh& m = world.get_component<mesh>(entity);
        m.vertex_buffer = get_submodule<ash::graphics::graphics>().make_vertex_buffer<vertex>(
            vertices.data(),
            vertices.size());
        m.index_buffer = get_submodule<ash::graphics::graphics>().make_index_buffer<std::int32_t>(
            indices.data(),
            indices.size());

        m_camera = world.create();
        world.add<main_camera, camera, transform>(m_camera);
        camera& c_camera = world.get_component<camera>(m_camera);
        c_camera.fov = math::to_radians(30.0f);
        c_camera.aspect = 1300.0f / 800.0f;
        c_camera.near_z = 0.01f;
        c_camera.far_z = 1000.0f;

        transform& c_transform = world.get_component<transform>(m_camera);
        c_transform.position = {10.0f, 10.0f, 10.0f};
        c_transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        c_transform.scaling = {1.0f, 1.0f, 1.0f};
        c_transform.node = std::make_unique<scene_node>();
        c_transform.parent = get_submodule<ash::scene::scene>().get_root_node();

        auto& task = get_submodule<task::task_manager>();
        auto root_handle = task.find("root");
        auto update_task = task.schedule("test update", [this]() { update(); });
        update_task->add_dependency(*root_handle);

        m_mouse = &get_submodule<ash::window::window>().get_mouse();
        m_keyboard = &get_submodule<ash::window::window>().get_keyboard();

        return true;
    }

    void update()
    {
        auto& world = get_submodule<ash::ecs::world>();

        transform& t = world.get_component<transform>(m_camera);

        if (m_keyboard->key(keyboard_key::KEY_W).down())
        {
            t.position[1] += 0.1f;
            t.node->dirty = true;
        }
    }

private:
    std::string m_title;

    keyboard* m_keyboard;
    mouse* m_mouse;

    entity_id m_camera;
};
} // namespace ash::sample::mmd

int main()
{
    ash::math::float4_simd t = ash::math::simd::set(1.0, 2.0f, 3.0f, 0.0f);
    ash::math::float4_simd r = ash::math::simd::set(2.0, 2.0f, 3.0f, 3.0f);
    ash::math::float4_simd s = ash::math::simd::set(1.0, 2.0f, 3.0f, 0.0f);

    ash::math::float4x4_simd result = ash::math::affine_transform_matrix::make(t, r, s);

    application app;
    app.install<window>();
    app.install<scene>();
    app.install<graphics>();
    app.install<ash::sample::mmd::test_module>();

    app.run();

    return 0;
}