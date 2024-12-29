#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "gltf_loader.hpp"
#include "graphics/graphics_system.hpp"
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

        auto& window = get_system<window_system>();
        window.on_resize().add_task().set_execute(
            [this]()
            {
                resize();
            });
        window.on_destroy().add_task().set_execute(
            []()
            {
                engine::exit();
            });

        task_graph& task_graph = get_task_graph();
        task_group& update = task_graph.get_group("Update");

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
        initialize_skybox();
        initialize_scene();

        resize();

        return true;
    }

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

        rhi_ptr<rhi_texture> env_map = texture_loader::load(m_skybox_path);
        ibl_tool::generate_cube_map(env_map.get(), m_skybox_texture.get());
        ibl_tool::generate_ibl(
            m_skybox_texture.get(),
            m_skybox_irradiance.get(),
            m_skybox_prefilter.get());
    }

    void initialize_scene()
    {
        auto& world = get_world();

        m_skybox = world.create();
        world.add_component<transform_component, skybox_component, scene_component>(m_skybox);
        auto& skybox = world.get_component<skybox_component>(m_skybox);
        skybox.texture = m_skybox_texture.get();
        skybox.irradiance = m_skybox_irradiance.get();
        skybox.prefilter = m_skybox_prefilter.get();

        m_light = world.create();
        world.add_component<transform_component, light_component, scene_component>(m_light);

        auto& light_transform = world.get_component<transform_component>(m_light);
        light_transform.set_position({10.0f, 10.0f, 10.0f});
        light_transform.lookat({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

        auto& main_light = world.get_component<light_component>(m_light);
        main_light.type = LIGHT_DIRECTIONAL;
        main_light.color = {1.0f, 1.0f, 1.0f};

        m_camera = world.create();
        world.add_component<
            transform_component,
            camera_component,
            orbit_control_component,
            scene_component>(m_camera);

        auto& camera_transform = world.get_component<transform_component>(m_camera);
        camera_transform.set_position({0.0f, 0.0f, -10.0f});

        auto& main_camera = world.get_component<camera_component>(m_camera);
        main_camera.renderer = m_renderer.get();
        main_camera.render_targets = {m_swapchain.get()};

        gltf_loader loader(m_model_path);
        if (auto result = loader.load())
        {
            m_model_data = std::move(*result);

            std::vector<entity> entities;
            for (auto& node : m_model_data.nodes)
            {
                entity entity = world.create();
                world.add_component<transform_component, mesh_component, scene_component>(entity);

                auto& mesh_data = m_model_data.meshes[node.mesh];

                auto& entity_mesh = world.get_component<mesh_component>(entity);
                entity_mesh.geometry = m_model_data.geometries[mesh_data.geometry].get();
                for (auto& submesh_data : mesh_data.submeshes)
                {
                    entity_mesh.submeshes.push_back({
                        .vertex_offset = submesh_data.vertex_offset,
                        .index_offset = submesh_data.index_offset,
                        .index_count = submesh_data.index_count,
                        .material = m_model_data.materials[submesh_data.material].get(),
                    });
                }

                auto& entity_transform = world.get_component<transform_component>(entity);
                entity_transform.set_position(node.position);
                entity_transform.set_rotation(node.rotation);
                entity_transform.set_scale(node.scale);
            }
        }
    }

    void resize()
    {
        auto extent = get_system<window_system>().get_extent();

        render_device& device = render_device::instance();

        m_swapchain->resize(extent.width, extent.height);

        auto& main_camera = get_world().get_component<camera_component>(m_camera);
        main_camera.render_targets[0] = m_swapchain.get();
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

    entity m_skybox;
    entity m_light;
    entity m_camera;

    rhi_ptr<rhi_texture> m_skybox_texture;
    rhi_ptr<rhi_texture> m_skybox_irradiance;
    rhi_ptr<rhi_texture> m_skybox_prefilter;

    mesh_loader::scene_data m_model_data;

    rhi_ptr<rhi_swapchain> m_swapchain;
    std::unique_ptr<renderer> m_renderer;

    std::string m_skybox_path;
    std::string m_model_path;
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
    violet::engine::install<violet::graphics_system>();
    violet::engine::install<violet::control_system>();
    violet::engine::install<violet::sample::pbr_sample>();

    violet::engine::run();

    return 0;
}