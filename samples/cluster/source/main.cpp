#include "components/camera_component.hpp"
#include "components/hierarchy_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "deferred_renderer_imgui.hpp"
#include "gltf_loader.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include "imgui.h"
#include "imgui_system.hpp"
#include "math/box.hpp"
#include "window/window_system.hpp"

namespace violet
{
class cluster_demo : public system
{
public:
    cluster_demo()
        : system("Cluster Demo")
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
            .set_name("Demo Tick")
            .set_group(update)
            .set_options(TASK_OPTION_MAIN_THREAD)
            .set_execute(
                [this]()
                {
                    tick();
                });

        initialize_render();
        initialize_scene(config["model"]);

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

    void initialize_scene(std::string_view model_path)
    {
        auto& world = get_world();

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

        auto& camera_control = world.get_component<orbit_control_component>(m_camera);
        camera_control.radius_speed = 0.05f;
        camera_control.target = {0.0f, 0.1f, 0.0f};

        auto& camera_transform = world.get_component<transform_component>(m_camera);
        camera_transform.set_position({0.0f, 0.0f, -10.0f});

        auto& main_camera = world.get_component<camera_component>(m_camera);
        main_camera.renderer = std::make_unique<deferred_renderer_imgui>();
        main_camera.render_target = m_swapchain.get();

        // Model.
        m_empty_material = std::make_unique<unlit_material>();

        m_root = world.create();
        world.add_component<transform_component, scene_component>(m_root);

        gltf_loader loader(model_path);
        if (auto result = loader.load())
        {
            m_model = std::move(*result);

            {
                auto positions = m_model.geometries[0]->get_position();
                auto indexes = m_model.geometries[0]->get_index();
                m_cluster_result = geometry_tool::generate_clusters(positions, indexes);

                m_cluster_geometry = std::make_unique<geometry>();
                m_cluster_geometry->set_position(m_cluster_result.positions);
                m_cluster_geometry->set_index(m_cluster_result.indexes);
            }

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
                            .material = submesh_data.material == -1 ?
                                            m_empty_material.get() :
                                            m_model.materials[submesh_data.material].get(),
                        });
                    }

                    entity_mesh.bounding_box = mesh_data.bounding_box;
                    entity_mesh.bounding_sphere = mesh_data.bounding_sphere;

                    m_mesh = entity;
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

        if (ImGui::CollapsingHeader("Cluster"))
        {
            bool dirty = false;
            static bool draw_cluster = false;
            if (ImGui::Checkbox("Draw Cluster", &draw_cluster))
            {
                dirty = true;

                if (draw_cluster)
                {
                    auto& mesh = world.get_component<mesh_component>(m_mesh);
                    mesh.geometry = m_cluster_geometry.get();
                }
                else
                {
                    auto& mesh = world.get_component<mesh_component>(m_mesh);
                    mesh.geometry = m_model.geometries[0].get();
                    mesh.submeshes.clear();
                    mesh.submeshes.push_back({
                        .index_count =
                            static_cast<std::uint32_t>(m_model.meshes[0].submeshes[0].index_count),
                        .material = m_empty_material.get(),
                    });
                }
            }

            if (draw_cluster)
            {
                static bool draw_group = false;
                if (ImGui::Checkbox("Draw Group", &draw_group))
                {
                    dirty = true;
                }

                static int lod = 0;
                if (ImGui::SliderInt(
                        "LOD",
                        &lod,
                        0,
                        static_cast<int>(m_cluster_result.lods.size() - 1)))
                {
                    dirty = true;
                }

                if (dirty)
                {
                    auto& mesh = world.get_component<mesh_component>(m_mesh);
                    mesh.geometry = m_cluster_geometry.get();
                    mesh.submeshes.clear();

                    for (std::uint32_t i = 0; i < m_cluster_result.lods[lod].group_count; ++i)
                    {
                        std::uint32_t group_index = m_cluster_result.lods[lod].group_offset + i;

                        for (std::uint32_t j = 0;
                             j < m_cluster_result.groups[group_index].cluster_count;
                             ++j)
                        {
                            std::uint32_t cluster_index =
                                m_cluster_result.groups[group_index].cluster_offset + j;

                            auto& cluster = m_cluster_result.clusters[cluster_index];

                            mesh.submeshes.push_back({
                                .index_offset = cluster.index_offset,
                                .index_count = cluster.index_count,
                                .material = get_material(draw_group ? group_index : cluster_index),
                            });
                        }
                    }
                }
            }
        }

#ifndef NDEBUG
        static bool draw_aabb = false;
        ImGui::Checkbox("Draw AABB", &draw_aabb);

        if (draw_aabb)
        {
            auto& debug_drawer = get_system<graphics_system>().get_debug_drawer();

            world.get_view().read<mesh_component>().read<transform_world_component>().each(
                [&](const mesh_component& mesh, const transform_world_component& transform)
                {
                    box3f box = box::transform(mesh.bounding_box, transform.matrix);
                    debug_drawer.draw_box(box, {0.0f, 1.0f, 0.0f});
                });
        }
#endif
    }

    unlit_material* get_material(std::uint32_t id)
    {
        auto to_color = [](std::uint32_t id) -> vec3f
        {
            std::uint32_t hash = id + 1;
            hash ^= hash >> 16;
            hash *= 0x85ebca6b;
            hash ^= hash >> 13;
            hash *= 0xc2b2ae35;
            hash ^= hash >> 16;

            return {
                static_cast<float>((hash >> 0) & 255) / 255.0f,
                static_cast<float>((hash >> 8) & 255) / 255.0f,
                static_cast<float>((hash >> 16) & 255) / 255.0f,
            };
        };

        if (m_materials.find(id) == m_materials.end())
        {
            auto material = std::make_unique<unlit_material>();
            // auto material = std::make_unique<unlit_material>(
            //     RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            //     RHI_CULL_MODE_NONE,
            //     RHI_POLYGON_MODE_LINE);
            material->set_color(to_color(id));
            m_materials[id] = std::move(material);
        }

        return m_materials[id].get();
    }

    entity m_light;
    entity m_camera;
    entity m_root;
    entity m_mesh;

    std::unique_ptr<unlit_material> m_empty_material;

    std::unordered_map<std::uint32_t, std::unique_ptr<unlit_material>> m_materials;

    mesh_loader::scene_data m_model;
    geometry_tool::cluster_result m_cluster_result;

    std::unique_ptr<geometry> m_cluster_geometry;

    rhi_ptr<rhi_swapchain> m_swapchain;

    application* m_app{nullptr};
};
} // namespace violet

int main()
{
    violet::application app("assets/config/cluster.json");
    app.install<violet::cluster_demo>();
    app.run();

    return 0;
}