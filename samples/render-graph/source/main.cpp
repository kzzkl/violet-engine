#include "components/camera_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/tools/ibl_tool.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "task/task_graph_printer.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class hello_world : public engine_system
{
public:
    hello_world()
        : engine_system("hello_world")
    {
    }

    bool initialize(const dictionary& config) override
    {
        auto& window = get_system<window_system>();
        window.on_resize().add_task().set_execute(
            [this]()
            {
                resize();
            });

        initialize_render();
        initialize_skybox();
        initialize_scene();

        resize();

        task_graph& task_graph = get_task_graph();
        task_graph.reset();
        task_graph_printer::print(task_graph);

        return true;
    }

    void shutdown() override {}

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

    void initialize_skybox()
    {
        m_skybox_texture = render_device::instance().create_texture({
            .extent = {512, 512},
            .format = RHI_FORMAT_R11G11B10_FLOAT,
            .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
            .layer_count = 6,
        });

        m_skybox_irradiance = render_device::instance().create_texture({
            .extent = {32, 32},
            .format = RHI_FORMAT_R11G11B10_FLOAT,
            .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
            .layer_count = 6,
        });

        m_skybox_prefilter = render_device::instance().create_texture({
            .extent = {128, 128},
            .format = RHI_FORMAT_R11G11B10_FLOAT,
            .flags = RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_CUBE,
            .level_count = 8,
            .layer_count = 6,
        });

        rhi_ptr<rhi_texture> env_map = texture_loader::load(
            "C://Workspace/violet-assets/textures/skybox/dry_orchard_meadow_4k.hdr");
        ibl_tool::generate_cube_map(env_map.get(), m_skybox_texture.get());
        ibl_tool::generate_ibl(
            m_skybox_texture.get(),
            m_skybox_irradiance.get(),
            m_skybox_prefilter.get());
    }

    void initialize_scene()
    {
        m_geometry = std::make_unique<box_geometry>();
        m_material = std::make_unique<pbr_material>();

        auto& world = get_world();

        m_skybox = world.create();
        world.add_component<
            transform_component,
            transform_local_component,
            transform_world_component,
            skybox_component,
            scene_component>(m_skybox);
        auto& skybox = world.get_component<skybox_component>(m_skybox);
        skybox.texture = m_skybox_texture.get();
        skybox.irradiance = m_skybox_irradiance.get();
        skybox.prefilter = m_skybox_prefilter.get();

        m_cube = world.create();
        world.add_component<
            transform_component,
            transform_local_component,
            transform_world_component,
            mesh_component,
            scene_component>(m_cube);

        auto& cube_mesh = world.get_component<mesh_component>(m_cube);
        cube_mesh.geometry = m_geometry.get();
        cube_mesh.submeshes.push_back({
            .vertex_offset = 0,
            .index_offset = 0,
            .index_count = m_geometry->get_index_count(),
            .material = m_material.get(),
        });

        m_camera = world.create();
        world.add_component<
            transform_component,
            transform_local_component,
            transform_world_component,
            camera_component,
            orbit_control_component,
            scene_component>(m_camera);

        auto& camera_transform = world.get_component<transform_component>(m_camera);
        camera_transform.position = {0.0f, 0.0f, -10.0f};

        auto& main_camera = world.get_component<camera_component>(m_camera);
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

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<material> m_material;

    entity m_cube;
    entity m_camera;
    entity m_skybox;

    rhi_ptr<rhi_texture> m_skybox_texture;
    rhi_ptr<rhi_texture> m_skybox_irradiance;
    rhi_ptr<rhi_texture> m_skybox_prefilter;
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
    violet::engine::install<violet::graphics_system>();
    violet::engine::install<violet::control_system>();
    violet::engine::install<violet::sample::hello_world>();

    violet::engine::run();

    return 0;
}