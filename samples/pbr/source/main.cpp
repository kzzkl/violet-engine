#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "deferred_renderer_imgui.hpp"
#include "ecs_command/ecs_command_system.hpp"
#include "gltf_loader.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/skybox.hpp"
#include "imgui.h"
#include "imgui_system.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "window/window_system.hpp"

namespace violet
{
class pbr_sample : public system
{
public:
    pbr_sample()
        : system("PBR Sample")
    {
    }

    void install(application& app) override
    {
        app.install<imgui_system>();

        m_app = &app;
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
            [this]()
            {
                m_app->exit();
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

        initialize_render();
        initialize_scene();

        resize();

        return true;
    }

private:
    void initialize_render()
    {
        m_swapchain = render_device::instance().create_swapchain({
            .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
            .window_handle = get_system<window_system>().get_handle(),
        });
        m_renderer = std::make_unique<deferred_renderer_imgui>();

        m_skybox = std::make_unique<skybox>(m_skybox_path);
    }

    void initialize_scene()
    {
        auto& world = get_world();

        entity scene_skybox = world.create();
        world.add_component<transform_component, skybox_component, scene_component>(scene_skybox);
        auto& skybox = world.get_component<skybox_component>(scene_skybox);
        skybox.skybox = m_skybox.get();

        m_light = world.create();
        world.add_component<transform_component, light_component, scene_component>(m_light);

        auto& light_transform = world.get_component<transform_component>(m_light);
        light_transform.set_position({10.0f, 10.0f, 10.0f});
        light_transform.lookat({0.0f, 0.0f, 0.0f});

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
        main_camera.render_targets = {
            m_swapchain.get(),
            m_taa_history[0].get(),
            m_taa_history[1].get(),
        };
        main_camera.jitter = true;

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
        render_device& device = render_device::instance();

        m_swapchain->resize();

        auto extent = get_system<window_system>().get_extent();

        for (auto& target : m_taa_history)
        {
            target = device.create_texture({
                .extent = {extent.width, extent.height},
                .format = RHI_FORMAT_R16G16B16A16_FLOAT,
                .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
                .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            });
        }

        auto& main_camera = get_world().get_component<camera_component>(m_camera);
        main_camera.render_targets[1] = m_taa_history[0].get();
        main_camera.render_targets[2] = m_taa_history[1].get();
    }

    void tick()
    {
        static bool enable_taa = true;
        if (ImGui::Checkbox("TAA", &enable_taa))
        {
            m_renderer->set_taa(enable_taa);

            auto& main_camera = get_world().get_component<camera_component>(m_camera);
            main_camera.jitter = enable_taa;
        }
    }

    entity m_light;
    entity m_camera;

    std::unique_ptr<skybox> m_skybox;

    mesh_loader::scene_data m_model_data;

    rhi_ptr<rhi_swapchain> m_swapchain;
    std::array<rhi_ptr<rhi_texture>, 2> m_taa_history;
    std::unique_ptr<deferred_renderer_imgui> m_renderer;

    std::string m_skybox_path;
    std::string m_model_path;

    application* m_app{nullptr};
};
} // namespace violet

int main()
{
    violet::application app("assets/config/pbr.json");
    app.install<violet::ecs_command_system>();
    app.install<violet::hierarchy_system>();
    app.install<violet::transform_system>();
    app.install<violet::scene_system>();
    app.install<violet::window_system>();
    app.install<violet::graphics_system>();
    app.install<violet::control_system>();
    app.install<violet::pbr_sample>();

    app.run();

    return 0;
}