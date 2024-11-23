#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/scene_layer.hpp"
#include "components/transform.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "fbx_loader.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/physical_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/mesh_system.hpp"
#include "graphics/renderers/default_renderer.hpp"
#include "graphics/tools/ibl_tool.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "task/task_graph_printer.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class pbr_sample : public engine_system
{
public:
    pbr_sample()
        : engine_system("PBR Sample")
    {
    }

    bool initialize(const dictionary& config) override
    {
        m_skybox_path = config["skybox"];
        m_model_path = config["model"];
        m_albedo_path = config["albedo_tex"];
        m_normal_path = config["normal_tex"];
        m_metallic_path = config["metallic_tex"];
        m_roughness_path = config["roughness_tex"];

        auto& window = get_system<window_system>();
        window.on_resize().add_task().set_execute(
            [this]()
            {
                resize();
            });
        window.on_destroy().add_task().set_execute(
            []()
            {
                // engine::exit();
            });

        task_graph& task_graph = get_task_graph();
        task_group& update = task_graph.get_group("Update Group");

        task_graph.add_task()
            .set_name("PBR Tick")
            .set_group(update)
            .set_execute(
                [this]()
                {
                    tick();
                });

        task_graph.reset();
        task_graph_printer::print(task_graph);

        initialize_render();
        initialize_scene();

        resize();

        return true;
    }

private:
    void initialize_render()
    {
        auto& device = render_device::instance();

        auto window_extent = get_system<window_system>().get_extent();

        rhi_swapchain_desc swapchain_desc = {};
        swapchain_desc.width = window_extent.width;
        swapchain_desc.height = window_extent.height;
        swapchain_desc.window_handle = get_system<window_system>().get_handle();
        m_swapchain = device.create_swapchain(swapchain_desc);
        m_renderer = std::make_unique<default_renderer>();
    }

    void initialize_scene()
    {
        m_scene = get_system<scene_system>().create_scene("Main Scene");

        auto& world = get_world();

        m_light = world.create();
        world.add_component<transform, light>(m_light);

        auto& light_transform = world.get_component<transform>(m_light);
        light_transform.position = {10.0f, 10.0f, 10.0f};
        light_transform.lookat({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

        auto& main_light = world.get_component<light>(m_light);
        main_light.type = LIGHT_DIRECTIONAL;
        main_light.color = {1.0f, 1.0f, 1.0f};

        vec3f model_center = {};

        // m_geometry = std::make_unique<box_geometry>();
        m_geometry = fbx_loader::load(m_model_path, &model_center);

        m_albedo = texture_loader::load(m_albedo_path, TEXTURE_LOAD_OPTION_SRGB);
        m_normal = texture_loader::load(m_normal_path);
        m_metallic = texture_loader::load(m_metallic_path);
        m_roughness = texture_loader::load(m_roughness_path);

        m_model = world.create();
        world.add_component<transform, transform_local, transform_world, mesh, scene_layer>(
            m_model);

        auto& cube_mesh = world.get_component<mesh>(m_model);
        cube_mesh.geometry = m_geometry.get();
        cube_mesh.submeshes.push_back({0, 0, 0, m_geometry->get_index_count(), {m_material.get()}});

        auto& cube_transform = world.get_component<transform>(m_model);
        cube_transform.position = {-model_center.x, -model_center.y, -model_center.z};

        auto& cube_scene = world.get_component<scene_layer>(m_model);
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

        auto& main_camera = world.get_component<camera>(m_camera);
        main_camera.renderer = m_renderer.get();
        main_camera.render_targets.resize(2);
        main_camera.viewport.min_depth = 0.0f;
        main_camera.viewport.max_depth = 1.0f;

        auto& camera_scene = world.get_component<scene_layer>(m_camera);
        camera_scene.scene = m_scene;
    }

    void resize()
    {
        auto extent = get_system<window_system>().get_extent();

        render_device& device = render_device::instance();

        m_swapchain->resize(extent.width, extent.height);

        rhi_texture_desc depth_buffer_desc = {
            .extent = {extent.width, extent.height},
            .format = RHI_FORMAT_D24_UNORM_S8_UINT,
            .level_count = 1,
            .layer_count = 1,
            .samples = RHI_SAMPLE_COUNT_1,
            .flags = RHI_TEXTURE_DEPTH_STENCIL};
        m_depth_buffer = device.create_texture(depth_buffer_desc);

        auto& main_camera = get_world().get_component<camera>(m_camera);
        main_camera.render_targets[0] = m_swapchain.get();
        main_camera.render_targets[1] = m_depth_buffer.get();
        main_camera.viewport.width = extent.width;
        main_camera.viewport.height = extent.height;

        main_camera.projection = matrix::perspective(
            45.0f,
            static_cast<float>(extent.width) / static_cast<float>(extent.height),
            0.1f,
            1000.0f);
    }

    void tick()
    {
        /*auto& camera_transform = get_world().get_component<const transform>(m_camera);
        log::debug(
            "{} {} {} {}",
            camera_transform.rotation.x,
            camera_transform.rotation.y,
            camera_transform.rotation.z,
            camera_transform.rotation.w);*/
    }

    rhi_ptr<rhi_swapchain> m_swapchain;
    rhi_ptr<rhi_texture> m_depth_buffer;

    rhi_ptr<rhi_texture> m_skybox;

    rhi_ptr<rhi_texture> m_albedo;
    rhi_ptr<rhi_texture> m_normal;
    rhi_ptr<rhi_texture> m_metallic;
    rhi_ptr<rhi_texture> m_roughness;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<physical_material> m_material;
    entity m_model;

    entity m_light;
    entity m_camera;

    scene* m_scene{nullptr};

    std::unique_ptr<renderer> m_renderer;

    std::string m_skybox_path;
    std::string m_model_path;
    std::string m_albedo_path;
    std::string m_normal_path;
    std::string m_metallic_path;
    std::string m_roughness_path;
};
} // namespace violet::sample

int main()
{
    violet::engine::initialize("pbr/config");
    violet::engine::install<violet::ecs_command_system>();
    violet::engine::install<violet::hierarchy_system>();
    violet::engine::install<violet::transform_system>();
    violet::engine::install<violet::scene_system>();
    violet::engine::install<violet::window_system>();
    violet::engine::install<violet::mesh_system>();
    violet::engine::install<violet::graphics_system>();
    violet::engine::install<violet::control_system>();
    violet::engine::install<violet::sample::pbr_sample>();

    violet::engine::run();

    return 0;
}