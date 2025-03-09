#include "components/camera_component.hpp"
#include "components/hierarchy_component.hpp"
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
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/physical_material.hpp"
#include "graphics/passes/gtao_pass.hpp"
#include "graphics/passes/taa_pass.hpp"
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
            .set_options(TASK_OPTION_MAIN_THREAD)
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
        main_camera.render_target = m_swapchain.get();
        main_camera.features.push_back(std::make_unique<taa_render_feature>());
        main_camera.features.push_back(std::make_unique<gtao_render_feature>());

        // Model.
        m_root = world.create();
        world.add_component<transform_component, scene_component>(m_root);

        gltf_loader loader(m_model_path);
        if (auto result = loader.load())
        {
            m_model = std::move(*result);

            std::vector<entity> entities;
            for (auto& node : m_model.nodes)
            {
                entity entity = world.create();
                world.add_component<transform_component, parent_component, scene_component>(entity);

                auto& transform = world.get_component<transform_component>(entity);
                transform.set_position(node.position);
                transform.set_rotation(node.rotation);
                transform.set_scale(node.scale);

                entities.push_back(entity);
            }

            for (std::size_t i = 0; i < m_model.nodes.size(); ++i)
            {
                auto& node = m_model.nodes[i];
                auto entity = entities[i];

                if (node.mesh != -1)
                {
                    world.add_component<mesh_component>(entity);

                    auto& mesh_data = m_model.meshes[node.mesh];

                    auto& entity_mesh = world.get_component<mesh_component>(entity);
                    entity_mesh.geometry = m_model.geometries[mesh_data.geometry].get();
                    for (auto& submesh_data : mesh_data.submeshes)
                    {
                        entity_mesh.submeshes.push_back({
                            .vertex_offset = submesh_data.vertex_offset,
                            .index_offset = submesh_data.index_offset,
                            .index_count = submesh_data.index_count,
                            .material = m_model.materials[submesh_data.material].get(),
                        });
                    }

                    entity_mesh.aabb = mesh_data.aabb;
                }

                if (node.parent != -1)
                {
                    world.get_component<parent_component>(entity).parent = entities[node.parent];
                }
                else
                {
                    world.get_component<parent_component>(entity).parent = m_root;
                }
            }
        }

        // Plane.
        m_plane = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_plane);

        m_plane_geometry = std::make_unique<box_geometry>(10.0f, 0.05f, 10.0f);
        m_plane_material = std::make_unique<physical_material>();

        auto& plane_mesh = world.get_component<mesh_component>(m_plane);
        plane_mesh.geometry = m_plane_geometry.get();
        plane_mesh.aabb = m_plane_geometry->get_aabb();
        plane_mesh.submeshes.push_back({
            .material = m_plane_material.get(),
        });
        auto& plane_transform = world.get_component<transform_component>(m_plane);
        plane_transform.set_position({0.0f, -1.0f, 0.0f});

        // Sphere.
        m_sphere = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_sphere);

        m_sphere_geometry = std::make_unique<sphere_geometry>();
        m_sphere_material = std::make_unique<physical_material>();

        auto& sphere_mesh = world.get_component<mesh_component>(m_sphere);
        sphere_mesh.geometry = m_sphere_geometry.get();
        sphere_mesh.aabb = m_sphere_geometry->get_aabb();
        sphere_mesh.submeshes.push_back({
            .material = m_sphere_material.get(),
        });
        auto& sphere_transform = world.get_component<transform_component>(m_sphere);
        sphere_transform.set_position({2.0f, 0.0f, 0.0f});
    }

    void resize()
    {
        m_swapchain->resize();
    }

    void tick()
    {
        auto& world = get_world();

        if (ImGui::CollapsingHeader("Transform"))
        {
            static float rotate = 0.0f;
            if (ImGui::SliderFloat("Rotate", &rotate, 0.0, 360.0))
            {
                auto& transform = world.get_component<transform_component>(m_root);
                transform.set_rotation(
                    quaternion::from_axis_angle(vec3f{0.0f, 1.0f, 0.0f}, math::to_radians(rotate)));
            }

            static float translate = 0.0f;
            if (ImGui::SliderFloat("Translate", &translate, -5.0f, 5.0))
            {
                auto& transform = world.get_component<transform_component>(m_root);
                transform.set_position({0.0f, 0.0f, translate});
            }
        }

        if (ImGui::CollapsingHeader("Material"))
        {
            static float metallic = m_sphere_material->get_metallic();
            static float roughness = m_sphere_material->get_roughness();
            static float albedo[] = {1.0f, 1.0f, 1.0f};

            if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
            {
                m_sphere_material->set_metallic(metallic);
            }

            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
            {
                m_sphere_material->set_roughness(roughness);
            }

            if (ImGui::ColorEdit3("Albedo", albedo))
            {
                m_sphere_material->set_albedo({albedo[0], albedo[1], albedo[2]});
            }
        }

        if (ImGui::CollapsingHeader("TAA"))
        {
            auto& main_camera = get_world().get_component<camera_component>(m_camera);
            auto* taa = main_camera.get_feature<taa_render_feature>();

            static bool enable_taa = taa->is_enable();

            ImGui::Checkbox("Enable##TAA", &enable_taa);

            taa->set_enable(enable_taa);
        }

        if (ImGui::CollapsingHeader("GTAO"))
        {
            auto& main_camera = get_world().get_component<camera_component>(m_camera);
            auto* gtao = main_camera.get_feature<gtao_render_feature>();

            static bool enable_gtao = gtao->is_enable();
            static int slice_count = static_cast<int>(gtao->get_slice_count());
            static int step_count = static_cast<int>(gtao->get_step_count());
            static float radius = gtao->get_radius();
            static float falloff = gtao->get_falloff();

            ImGui::Checkbox("Enable##GTAO", &enable_gtao);
            ImGui::SliderInt("Slice Count", &slice_count, 1, 5);
            ImGui::SliderInt("Step Count", &step_count, 1, 5);
            ImGui::SliderFloat("Radius", &radius, 0.0f, 10.0f);
            ImGui::SliderFloat("Falloff", &falloff, 0.1f, 1.0f);

            gtao->set_enable(enable_gtao);
            gtao->set_slice_count(slice_count);
            gtao->set_step_count(step_count);
            gtao->set_radius(radius);
            gtao->set_falloff(falloff);
        }

#ifndef NDEBUG
        static bool draw_aabb = false;
        ImGui::Checkbox("Draw AABB", &draw_aabb);

        if (draw_aabb)
        {
            auto& debug_drawer = get_system<graphics_system>().get_debug_drawer();
            auto& world = get_world();

            world.get_view().read<mesh_component>().read<transform_world_component>().each(
                [&](const mesh_component& mesh, const transform_world_component& transform)
                {
                    box3f box = box::transform(mesh.aabb, transform.matrix);
                    debug_drawer.draw_box(box, {0.0f, 1.0f, 0.0f});
                });
        }
#endif
    }

    entity m_light;
    entity m_camera;
    entity m_root;

    entity m_plane;
    std::unique_ptr<geometry> m_plane_geometry;
    std::unique_ptr<material> m_plane_material;

    entity m_sphere;
    std::unique_ptr<geometry> m_sphere_geometry;
    std::unique_ptr<physical_material> m_sphere_material;

    std::unique_ptr<skybox> m_skybox;

    mesh_loader::scene_data m_model;

    rhi_ptr<rhi_swapchain> m_swapchain;
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