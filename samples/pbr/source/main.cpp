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
#include "gltf_loader.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/physical_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/features/gtao_render_feature.hpp"
#include "graphics/renderers/features/taa_render_feature.hpp"
#include "graphics/skybox.hpp"
#include "imgui.h"
#include "imgui_system.hpp"
#include "window/window_system.hpp"

namespace violet
{
class pbr_sample : public system
{
public:
    pbr_sample()
        : system("pbr_sample")
    {
    }

    void install(application& app) override
    {
        app.install<window_system>();
        app.install<graphics_system>();
        app.install<control_system>();
        app.install<imgui_system>();

        m_app = &app;
    }

    bool initialize(const dictionary& config) override
    {
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
        initialize_scene(config["model"], config["skybox"]);

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
    }

    void initialize_scene(std::string_view model_path, std::string_view skybox_path)
    {
        auto& world = get_world();

        m_skybox = std::make_unique<skybox>(skybox_path);

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
        main_camera.renderer = std::make_unique<deferred_renderer_imgui>();
        main_camera.render_target = m_swapchain.get();

        // Model.
        m_empty_material = std::make_unique<unlit_material>();

        m_root = world.create();
        world.add_component<transform_component, scene_component>(m_root);

        load_model(model_path);

        // Plane.
        m_plane = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_plane);

        m_plane_geometry = std::make_unique<box_geometry>();

        m_plane_material = std::make_unique<physical_material>();
        m_plane_material->set_metallic(0.5f);
        m_plane_material->set_roughness(0.5f);

        auto& plane_mesh = world.get_component<mesh_component>(m_plane);
        plane_mesh.geometry = m_plane_geometry.get();
        plane_mesh.submeshes.push_back({
            .index = 0,
            .material = m_plane_material.get(),
        });
        auto& plane_transform = world.get_component<transform_component>(m_plane);
        plane_transform.set_position({0.0f, -1.0f, 0.0f});
        plane_transform.set_scale({10.0f, 0.05f, 10.0f});
    }

    void load_model(std::string_view path)
    {
        auto& world = get_world();

        gltf_loader loader;
        if (auto result = loader.load(path))
        {
            m_textures = std::move(result->textures);

            for (const auto& geometry_data : result->geometries)
            {
                auto model_geometry = std::make_unique<geometry>();
                model_geometry->set_positions(geometry_data.positions);
                model_geometry->set_normals(geometry_data.normals);
                model_geometry->set_tangents(geometry_data.tangents);
                model_geometry->set_texcoords(geometry_data.texcoords);
                model_geometry->set_indexes(geometry_data.indexes);

                for (const auto& submesh : geometry_data.submeshes)
                {
                    model_geometry->add_submesh(
                        submesh.vertex_offset,
                        submesh.index_offset,
                        submesh.index_count);
                }

                m_geometries.push_back(std::move(model_geometry));
            }

            for (const auto& material_data : result->materials)
            {
                auto model_material = std::make_unique<physical_material>();
                model_material->set_albedo(material_data.albedo);
                model_material->set_roughness(material_data.roughness);
                model_material->set_metallic(material_data.metallic);
                model_material->set_emissive(material_data.emissive);

                if (material_data.albedo_texture != nullptr)
                {
                    model_material->set_albedo(material_data.albedo_texture);
                }

                if (material_data.roughness_metallic_texture != nullptr)
                {
                    model_material->set_roughness_metallic(
                        material_data.roughness_metallic_texture);
                }

                if (material_data.emissive_texture != nullptr)
                {
                    model_material->set_emissive(material_data.emissive_texture);
                }

                if (material_data.normal_texture != nullptr)
                {
                    model_material->set_normal(material_data.normal_texture);
                }

                m_materials.push_back(std::move(model_material));
            }

            for (const auto& node_data : result->nodes)
            {
                entity entity = world.create();
                world.add_component<transform_component, parent_component, scene_component>(entity);

                auto& transform = world.get_component<transform_component>(entity);
                transform.set_position(node_data.position);
                transform.set_rotation(node_data.rotation);
                transform.set_scale(node_data.scale);

                m_entities.push_back(entity);
            }

            for (std::size_t i = 0; i < result->nodes.size(); ++i)
            {
                const auto& node_data = result->nodes[i];
                auto entity = m_entities[i];

                if (node_data.mesh != -1)
                {
                    world.add_component<mesh_component>(entity);

                    const auto& mesh_data = result->meshes[node_data.mesh];

                    auto& entity_mesh = world.get_component<mesh_component>(entity);
                    entity_mesh.geometry = m_geometries[mesh_data.geometry].get();

                    for (std::size_t j = 0; j < mesh_data.submeshes.size(); ++j)
                    {
                        entity_mesh.submeshes.push_back({
                            .index = mesh_data.submeshes[j],
                            .material = mesh_data.materials[j] == -1 ?
                                            m_empty_material.get() :
                                            m_materials[mesh_data.materials[j]].get(),
                        });
                    }
                }

                if (node_data.parent != -1)
                {
                    world.get_component<parent_component>(entity).parent =
                        m_entities[node_data.parent];
                }
                else
                {
                    world.get_component<parent_component>(entity).parent = m_root;
                }
            }
        }
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
            static float metallic = m_plane_material->get_metallic();
            static float roughness = m_plane_material->get_roughness();
            static float albedo[] = {1.0f, 1.0f, 1.0f};

            if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
            {
                m_plane_material->set_metallic(metallic);
            }

            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
            {
                m_plane_material->set_roughness(roughness);
            }

            if (ImGui::ColorEdit3("Albedo", albedo))
            {
                m_plane_material->set_albedo({albedo[0], albedo[1], albedo[2]});
            }
        }

        if (ImGui::CollapsingHeader("TAA"))
        {
            auto& main_camera = get_world().get_component<camera_component>(m_camera);
            auto* taa = main_camera.renderer->get_feature<taa_render_feature>();

            static bool enable_taa = taa->is_enable();

            ImGui::Checkbox("Enable##TAA", &enable_taa);

            if (enable_taa)
            {
                taa->enable();
            }
            else
            {
                taa->disable();
            }
        }

        if (ImGui::CollapsingHeader("GTAO"))
        {
            auto& main_camera = get_world().get_component<camera_component>(m_camera);
            auto* gtao = main_camera.renderer->get_feature<gtao_render_feature>();

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

            if (enable_gtao)
            {
                gtao->enable();
            }
            else
            {
                gtao->disable();
            }
            gtao->set_slice_count(slice_count);
            gtao->set_step_count(step_count);
            gtao->set_radius(radius);
            gtao->set_falloff(falloff);
        }
    }

    entity m_light;
    entity m_camera;
    entity m_root;

    entity m_plane;
    std::unique_ptr<geometry> m_plane_geometry;
    std::unique_ptr<physical_material> m_plane_material;

    std::vector<std::unique_ptr<geometry>> m_geometries;
    std::vector<std::unique_ptr<material>> m_materials;
    std::vector<std::unique_ptr<texture_2d>> m_textures;
    std::vector<entity> m_entities;

    std::unique_ptr<unlit_material> m_empty_material;

    std::unique_ptr<skybox> m_skybox;

    rhi_ptr<rhi_swapchain> m_swapchain;

    application* m_app{nullptr};
};
} // namespace violet

int main()
{
    violet::application app("assets/config/pbr.json");
    app.install<violet::pbr_sample>();
    app.run();

    return 0;
}