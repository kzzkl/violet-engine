#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/scene_layer.hpp"
#include "components/transform.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "graphics/camera_system.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/mesh_system.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "task/task_graph_printer.hpp"
#include "window/window_system.hpp"
#include <filesystem>
#include <fstream>
#include <thread>

namespace violet::sample
{
class hello_world : public engine_system
{
public:
    hello_world()
        : engine_system("hello_world")
    {
    }

    virtual bool initialize(const dictionary& config)
    {
        auto& window = get_system<window_system>();
        window.on_resize().add_task().set_execute(
            [this]()
            {
                resize();
            });

        initialize_render();
        initialize_scene();

        resize();

        task_graph& task_graph = get_task_graph();
        task_graph.reset();
        task_graph_printer::print(task_graph);

        return true;
    }

    virtual void shutdown() {}

private:
    void initialize_render()
    {
        auto window_extent = get_system<window_system>().get_extent();

        m_swapchain = render_device::instance().create_swapchain({
            .extent =
                {
                    .width = window_extent.width,
                    .height = window_extent.height,
                },
            .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
            .window_handle = get_system<window_system>().get_handle(),
        });
        m_renderer = std::make_unique<deferred_renderer>();
    }

    void initialize_scene()
    {
        m_scene = get_system<scene_system>().create_scene("Main Scene");

        m_geometry = std::make_unique<box_geometry>();
        m_material = std::make_unique<unlit_material>();

        auto& world = get_world();

        m_cube = world.create();
        world.add_component<transform, transform_local, transform_world, mesh, scene_layer>(m_cube);

        auto& cube_mesh = world.get_component<mesh>(m_cube);
        cube_mesh.geometry = m_geometry.get();
        cube_mesh.submeshes.push_back({
            .vertex_offset = 0,
            .index_offset = 0,
            .index_count = m_geometry->get_index_count(),
            .material = m_material.get(),
        });

        auto& cube_scene = world.get_component<scene_layer>(m_cube);
        cube_scene.scene = m_scene;

        m_camera = world.create();
        world.add_component<
            transform,
            transform_local,
            transform_world,
            camera,
            orbit_control,
            scene_layer>(m_camera);

        auto& camera_transform = world.get_component<transform>(m_camera);
        camera_transform.position = {0.0f, 0.0f, -10.0f};

        auto& camera_scene = world.get_component<scene_layer>(m_camera);
        camera_scene.scene = m_scene;

        auto& main_camera = world.get_component<camera>(m_camera);
        main_camera.near = 1.0f;
        main_camera.far = 1000.0f;
        main_camera.fov = 45.0f;
        main_camera.render_targets = {m_swapchain.get()};
        main_camera.renderer = m_renderer.get();
    }

    void resize()
    {
        auto extent = get_system<window_system>().get_extent();
        m_swapchain->resize(extent.width, extent.height);
    }

    std::unique_ptr<deferred_renderer> m_renderer;
    rhi_ptr<rhi_swapchain> m_swapchain;

    scene* m_scene;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<material> m_material;

    entity m_cube;
    entity m_camera;
};
} // namespace violet::sample

int main()
{
    violet::engine::initialize("render-graph/config");
    violet::engine::install<violet::ecs_command_system>();
    violet::engine::install<violet::hierarchy_system>();
    violet::engine::install<violet::transform_system>();
    violet::engine::install<violet::scene_system>();
    violet::engine::install<violet::window_system>();
    violet::engine::install<violet::mesh_system>();
    violet::engine::install<violet::camera_system>();
    violet::engine::install<violet::graphics_system>();
    violet::engine::install<violet::control_system>();
    violet::engine::install<violet::sample::hello_world>();

    violet::engine::run();

    return 0;
}