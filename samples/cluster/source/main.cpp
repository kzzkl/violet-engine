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
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "imgui.h"
#include "imgui_system.hpp"
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
        camera_control.radius_speed = 0.2f;
        camera_control.target = {0.0f, 0.1f, 0.0f};

        auto& camera_transform = world.get_component<transform_component>(m_camera);
        camera_transform.set_position({0.0f, 0.0f, -10.0f});

        auto& main_camera = world.get_component<camera_component>(m_camera);
        main_camera.renderer = std::make_unique<deferred_renderer_imgui>();
        main_camera.render_target = m_swapchain.get();

        // Model.
        m_empty_material = std::make_unique<unlit_material>();

        if (!model_path.empty())
        {
            load_model(model_path);
        }
        else
        {
            m_empty_geometry = std::make_unique<sphere_geometry>(0.5f, 128, 64);

            m_mesh = world.create();
            world.add_component<transform_component, mesh_component, scene_component>(m_mesh);
            auto& mesh = world.get_component<mesh_component>(m_mesh);
            mesh.geometry = m_empty_geometry.get();
        }

        auto& mesh = world.get_component<mesh_component>(m_mesh);
        mesh.geometry->generate_clusters();
        mesh.geometry->clear_submeshes();
        mesh.submeshes.clear();

        for (std::uint32_t i = 0; i < mesh.geometry->get_clusters().size(); ++i)
        {
            const auto& cluster = mesh.geometry->get_clusters()[i];

            m_max_lod = std::max(m_max_lod, cluster.lod);
            mesh.geometry->add_submesh(0, cluster.index_offset, cluster.index_count);

            if (cluster.lod == 0)
            {
                mesh.submeshes.push_back({
                    .index = i,
                    .material = get_material(i),
                });
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

        static bool auto_lod = false;
        ImGui::Checkbox("Auto LOD", &auto_lod);

        if (auto_lod)
        {
            std::uint32_t triangle_count = cull_cluster(1.0f);
            ImGui::Text("Triangle Count: %u", triangle_count);
        }
        else
        {
            bool dirty = false;

            static int lod = 0;
            if (ImGui::SliderInt("LOD", &lod, 0, static_cast<int>(m_max_lod)))
            {
                dirty = true;
            }

            if (dirty)
            {
                auto& mesh = world.get_component<mesh_component>(m_mesh);
                mesh.submeshes.clear();

                const auto& clusters = mesh.geometry->get_clusters();
                for (std::uint32_t i = 0; i < clusters.size(); ++i)
                {
                    if (clusters[i].lod != lod)
                    {
                        continue;
                    }

                    mesh.submeshes.push_back({
                        .index = i,
                        .material = get_material(i),
                    });
                }
            }
        }
    }

    void load_model(std::string_view path)
    {
        auto& world = get_world();

        gltf_loader loader(path);

        auto result = loader.load();
        if (!result)
        {
            return;
        }

        m_model = std::move(*result);

        std::vector<entity> entities;
        for (auto& node : m_model.nodes)
        {
            entity entity = world.create();
            world.add_component<transform_component, scene_component>(entity);

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
                        .index = submesh_data.submesh_index,
                        .material = submesh_data.material == -1 ?
                                        m_empty_material.get() :
                                        m_model.materials[submesh_data.material].get(),
                    });
                }

                m_mesh = entity;
            }

            if (node.parent != -1)
            {
                world.add_component<parent_component>(entity);
                world.get_component<parent_component>(entity).parent = entities[node.parent];
            }
        }
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

    std::uint32_t cull_cluster(float throshold)
    {
        const auto& camera = get_world().get_component<const camera_component>(m_camera);
        float height = static_cast<float>(camera.get_extent().height);
        float fov = camera.fov;
        float cot_half_fov = 1.0f / std::tan(fov * 0.5f);

        const auto& camera_world =
            get_world().get_component<const transform_world_component>(m_camera);
        mat4f matrix_v = matrix::inverse_transform(camera_world.matrix);

        const auto& mesh_world = get_world().get_component<const transform_world_component>(m_mesh);
        mat4f matrix_m = mesh_world.matrix;
        mat4f matrix_mv = matrix::mul(matrix_m, matrix_v);

        auto& mesh = get_world().get_component<mesh_component>(m_mesh);
        const auto& clusters = mesh.geometry->get_clusters();

        auto project_error_to_screen = [&](const sphere3f& sphere)
        {
            if (!std::isfinite(sphere.radius))
            {
                return sphere.radius;
            }
            const float d2 = vector::dot(sphere.center, sphere.center);
            const float r = sphere.radius;
            return height / 2.0f * cot_half_fov * r / std::sqrt(d2 - r * r);
        };

        std::vector<std::uint32_t> visible_clusters;
        std::vector<std::uint8_t> visible_flags(clusters.size());

        std::queue<std::uint32_t> queue;
        queue.push(clusters.size() - 1);

        while (!queue.empty())
        {
            std::uint32_t cluster_index = queue.front();
            queue.pop();

            if (visible_flags[cluster_index])
            {
                continue;
            }

            visible_flags[cluster_index] = 1;

            const auto& cluster = clusters[cluster_index];

            sphere3f project_bounds = {
                .center = matrix::mul(
                    vec4f{
                        cluster.lod_bounds.center.x,
                        cluster.lod_bounds.center.y,
                        cluster.lod_bounds.center.z,
                        1.0f},
                    matrix_mv),
                .radius = cluster.lod_error,
            };
            float project_error = project_error_to_screen(project_bounds);

            sphere3f project_parent_bounds = {
                .center = matrix::mul(
                    vec4f{
                        cluster.parent_lod_bounds.center.x,
                        cluster.parent_lod_bounds.center.y,
                        cluster.parent_lod_bounds.center.z,
                        1.0f},
                    matrix_mv),
                .radius = cluster.parent_lod_error,
            };
            float project_parent_error = project_error_to_screen(project_parent_bounds);

            if ((project_error < throshold && project_parent_error > throshold) ||
                cluster.lod_error == 0.0f)
            {
                visible_clusters.push_back(cluster_index);
            }
            else
            {
                for (std::uint32_t i = 0; i < cluster.children_count; ++i)
                {
                    queue.push(cluster.children_offset + i);
                }
            }
        }

        mesh.submeshes.clear();

        std::uint32_t triangle_count = 0;
        for (std::uint32_t cluster_index : visible_clusters)
        {
            mesh.submeshes.push_back({
                .index = cluster_index,
                .material = get_material(cluster_index),
            });

            triangle_count += clusters[cluster_index].index_count / 3;
        }

        return triangle_count;
    }

    entity m_light;
    entity m_camera;
    entity m_mesh;

    mesh_loader::scene_data m_model;

    std::unique_ptr<unlit_material> m_empty_material;
    std::unique_ptr<geometry> m_empty_geometry;

    std::unordered_map<std::uint32_t, std::unique_ptr<unlit_material>> m_materials;

    std::uint32_t m_max_lod{0};

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