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

        m_bvh_node_count =
            static_cast<std::uint32_t>(mesh.geometry->get_cluster_bvh_nodes().size());

        // Debug.
        m_debug_geometry = std::make_unique<sphere_geometry>(1.0f, 64, 32);
        m_debug_material = std::make_unique<unlit_material>();
        m_debug_material->set_color({1.0f, 0.0f, 0.0f});

        for (std::uint32_t i = 0; i < mesh.geometry->get_clusters().size(); ++i)
        {
            entity entity = world.create();
            world.add_component<
                transform_component,
                mesh_component,
                scene_component,
                parent_component>(entity);

            auto& debug_mesh = world.get_component<mesh_component>(entity);
            debug_mesh.geometry = m_debug_geometry.get();

            auto& debug_parent = world.get_component<parent_component>(entity);
            debug_parent.parent = m_mesh;

            m_debug_entities.push_back(entity);
        }
    }

    void resize()
    {
        m_swapchain->resize();
    }

    void tick()
    {
        auto& world = get_world();

        const char* items[] = {"Auto LOD", "LOD", "BVH"};
        static int item = 0;
        ImGui::Combo("View", &item, items, IM_ARRAYSIZE(items));

        if (item == 0)
        {
            const char* cull_modes[] = {"Hierarchy", "BVH", "Naive"};
            static int mode = 0;

            ImGui::Combo("Mode", &mode, cull_modes, IM_ARRAYSIZE(cull_modes));

            static bool lod_forced = false;
            ImGui::Checkbox("Force LOD", &lod_forced);

            static int lod = 0;
            if (lod_forced)
            {
                ImGui::SliderInt("LOD", &lod, 0, static_cast<int>(m_max_lod));
            }

            std::uint32_t triangle_count = cull_cluster(1.0f, mode, lod_forced ? &lod : nullptr);
            ImGui::Text("Triangle Count: %u", triangle_count);
            ImGui::Text("Checked Count: %u", m_check_times);
        }
        else if (item == 1)
        {
            static int lod = 0;
            if (ImGui::SliderInt("LOD", &lod, 0, static_cast<int>(m_max_lod)))
            {
                show_lod(lod);
            }
        }
        else if (item == 2)
        {
            static int bvh_node_index = 0;
            if (ImGui::SliderInt(
                    "BVH Node",
                    &bvh_node_index,
                    0,
                    static_cast<int>(m_bvh_node_count - 1)))
            {
                show_bvh_node(bvh_node_index);
            }
        }

        static bool debug = false;
        if (ImGui::Checkbox("Error Bounds", &debug))
        {
            if (!debug)
            {
                for (entity entity : m_debug_entities)
                {
                    auto& debug_mesh = world.get_component<mesh_component>(entity);
                    debug_mesh.submeshes.clear();
                }
            }
        }

        if (debug)
        {
            static bool debug_view = false;
            static int debug_lod = 0;
            static float error_bounds_scale = 1.0f;

            ImGui::Checkbox("Debug View", &debug_view);
            ImGui::SliderInt("Debug LOD", &debug_lod, 0, static_cast<int>(m_max_lod));
            ImGui::SliderFloat("Error Bounds Scale", &error_bounds_scale, 1.0f, 10.0f);

            for (entity entity : m_debug_entities)
            {
                auto& debug_mesh = world.get_component<mesh_component>(entity);
                debug_mesh.submeshes.clear();
            }

            const auto& mesh_world = world.get_component<const transform_world_component>(m_mesh);

            const auto& camera_world =
                world.get_component<const transform_world_component>(m_camera);
            vec3f camera_position = camera_world.get_position();
            camera_position = matrix::mul(
                {camera_position.x, camera_position.y, camera_position.z, 1.0f},
                matrix::inverse(mesh_world.matrix));

            const auto& mesh = world.get_component<mesh_component>(m_mesh);
            const auto& clusters = mesh.geometry->get_clusters();

            for (std::uint32_t i = 0; i < clusters.size(); ++i)
            {
                if (clusters[i].lod != debug_lod)
                {
                    continue;
                }

                auto& debug_mesh = world.get_component<mesh_component>(m_debug_entities[i]);
                debug_mesh.submeshes.push_back({
                    .index = 0,
                    .material = m_debug_material.get(),
                });

                auto& debug_transform =
                    world.get_component<transform_component>(m_debug_entities[i]);

                if (debug_view)
                {
                    vec3f camera_to_center = clusters[i].lod_bounds.center - camera_position;
                    vec3f near =
                        clusters[i].lod_bounds.center -
                        vector::normalize(camera_to_center) * clusters[i].lod_bounds.radius;

                    debug_transform.set_position(near);
                    debug_transform.set_scale(clusters[i].lod_error * error_bounds_scale);
                }
                else
                {
                    debug_transform.set_position(clusters[i].lod_bounds.center);
                    debug_transform.set_scale(clusters[i].lod_bounds.radius * error_bounds_scale);
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

    std::uint32_t cull_cluster(float throshold, int mode, const int* lod = nullptr)
    {
        const auto& camera = get_world().get_component<const camera_component>(m_camera);
        float height = static_cast<float>(camera.get_extent().height);
        float fov = camera.fov;
        float cot_half_fov = 1.0f / std::tan(fov * 0.5f);

        const auto& camera_world =
            get_world().get_component<const transform_world_component>(m_camera);
        mat4f matrix_v = matrix::inverse_transform(camera_world.matrix);

        const auto& mesh_world = get_world().get_component<const transform_world_component>(m_mesh);
        mat4f matrix_mv = matrix::mul(mesh_world.matrix, matrix_v);

        auto project_error_to_screen = [&](const sphere3f& bounds, float error) -> vec2f
        {
            if (!std::isfinite(error))
            {
                return {error, error};
            }

            vec3f center_vs = matrix::mul(
                vec4f{bounds.center.x, bounds.center.y, bounds.center.z, 1.0f},
                matrix_mv);
            vec3f direction_vs = vector::normalize(center_vs);
            vec3f near_vs = center_vs - direction_vs * bounds.radius;
            vec3f far_vs = center_vs + direction_vs * bounds.radius;

            const float near_d2 = vector::dot(near_vs, near_vs);
            const float far_d2 = vector::dot(far_vs, far_vs);

            float error2 = error * error;

            return {
                near_d2 > error2 ?
                    height / 2.0f * cot_half_fov * error / std::sqrt(near_d2 - error2) :
                    std::numeric_limits<float>::infinity(),
                far_d2 > error2 ?
                    height / 2.0f * cot_half_fov * error / std::sqrt(far_d2 - error2) :
                    std::numeric_limits<float>::infinity(),
            };
        };

        std::vector<std::uint32_t> visible_clusters;

        m_check_times = 0;

        switch (mode)
        {
        case 0:
            visible_clusters = cull_cluster_hierarchy(throshold, project_error_to_screen);
            break;
        case 1:
            visible_clusters = cull_cluster_bvh(throshold, project_error_to_screen);
            break;
        case 2:
            visible_clusters = cull_cluster_naive(throshold, project_error_to_screen);
            break;
        }

        auto& mesh = get_world().get_component<mesh_component>(m_mesh);
        mesh.submeshes.clear();

        const auto& clusters = mesh.geometry->get_clusters();

        std::uint32_t triangle_count = 0;
        for (std::uint32_t cluster_index : visible_clusters)
        {
            if (lod != nullptr && clusters[cluster_index].lod != *lod)
            {
                continue;
            }

            mesh.submeshes.push_back({
                .index = cluster_index,
                .material = get_material(cluster_index),
            });

            triangle_count += clusters[cluster_index].index_count / 3;
        }

        return triangle_count;
    }

    template <typename Functor>
    std::vector<std::uint32_t> cull_cluster_hierarchy(
        float throshold,
        Functor&& project_error_to_screen)
    {
        const auto& mesh = get_world().get_component<const mesh_component>(m_mesh);
        const auto& clusters = mesh.geometry->get_clusters();

        std::vector<std::uint32_t> visible_clusters;
        std::vector<std::uint8_t> visible_flags(clusters.size());

        std::queue<std::uint32_t> queue;
        queue.push(clusters.size() - 1);

        while (!queue.empty())
        {
            std::uint32_t index = queue.front();
            queue.pop();

            if (visible_flags[index])
            {
                continue;
            }

            visible_flags[index] = 1;

            const auto& cluster = clusters[index];

            float project_error = project_error_to_screen(cluster.lod_bounds, cluster.lod_error).x;
            float project_parent_error =
                project_error_to_screen(cluster.parent_lod_bounds, cluster.parent_lod_error).x;

            if (project_error < throshold && project_parent_error > throshold)
            {
                visible_clusters.push_back(index);
            }
            else
            {
                for (std::uint32_t i = 0; i < cluster.children_count; ++i)
                {
                    queue.push(cluster.children_offset + i);
                }
            }

            ++m_check_times;
        }

        return visible_clusters;
    }

    template <typename Functor>
    std::vector<std::uint32_t> cull_cluster_bvh(float throshold, Functor&& project_error_to_screen)
    {
        const auto& mesh = get_world().get_component<const mesh_component>(m_mesh);
        const auto& bvh_nodes = mesh.geometry->get_cluster_bvh_nodes();

        std::vector<std::uint32_t> candidate_clusters;

        std::queue<std::uint32_t> queue;
        queue.push(0);

        while (!queue.empty())
        {
            std::uint32_t index = queue.front();
            queue.pop();

            const auto& node = bvh_nodes[index];

            float min_error = project_error_to_screen(node.lod_bounds, node.min_lod_error).y;
            float max_parent_error =
                project_error_to_screen(node.lod_bounds, node.max_parent_lod_error).x;

            if (min_error < throshold && max_parent_error > throshold)
            {
                if (node.is_leaf)
                {
                    for (std::uint32_t child : node.children)
                    {
                        candidate_clusters.push_back(child);
                    }
                }
                else
                {
                    for (std::uint32_t child : node.children)
                    {
                        queue.push(child);
                    }
                }
            }

            ++m_check_times;
        }

        std::vector<std::uint32_t> visible_clusters;
        for (std::uint32_t cluster_index : candidate_clusters)
        {
            const auto& cluster = mesh.geometry->get_clusters()[cluster_index];

            float project_error = project_error_to_screen(cluster.lod_bounds, cluster.lod_error).x;
            float project_parent_error =
                project_error_to_screen(cluster.parent_lod_bounds, cluster.parent_lod_error).x;

            if (project_error < throshold && project_parent_error > throshold)
            {
                visible_clusters.push_back(cluster_index);
            }

            ++m_check_times;
        }

        return visible_clusters;
    }

    template <typename Functor>
    std::vector<std::uint32_t> cull_cluster_naive(
        float throshold,
        Functor&& project_error_to_screen)
    {
        const auto& mesh = get_world().get_component<const mesh_component>(m_mesh);
        const auto& clusters = mesh.geometry->get_clusters();

        std::vector<std::uint32_t> visible_clusters;
        for (std::uint32_t i = 0; i < clusters.size(); ++i)
        {
            const auto& cluster = clusters[i];

            float project_error = project_error_to_screen(cluster.lod_bounds, cluster.lod_error).x;
            float project_parent_error =
                project_error_to_screen(cluster.parent_lod_bounds, cluster.parent_lod_error).x;

            if (project_error < throshold && project_parent_error > throshold)
            {
                visible_clusters.push_back(i);
            }

            ++m_check_times;
        }

        return visible_clusters;
    }

    void show_lod(std::uint32_t lod)
    {
        auto& mesh = get_world().get_component<mesh_component>(m_mesh);
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

    void show_bvh_node(std::uint32_t bvh_node_index)
    {
        auto& mesh = get_world().get_component<mesh_component>(m_mesh);
        const auto& bvh_nodes = mesh.geometry->get_cluster_bvh_nodes();

        mesh.submeshes.clear();

        std::queue<std::uint32_t> queue;
        queue.push(bvh_node_index);

        while (!queue.empty())
        {
            std::uint32_t index = queue.front();
            queue.pop();

            const auto& bvh_node = bvh_nodes[index];

            if (bvh_node.is_leaf)
            {
                for (std::uint32_t child : bvh_node.children)
                {
                    mesh.submeshes.push_back({
                        .index = child,
                        .material = get_material(child),
                    });
                }
            }
            else
            {
                for (std::uint32_t child : bvh_node.children)
                {
                    queue.push(child);
                }
            }
        }
    }

    entity m_light;
    entity m_camera;
    entity m_mesh;

    mesh_loader::scene_data m_model;

    std::unique_ptr<unlit_material> m_empty_material;
    std::unique_ptr<geometry> m_empty_geometry;

    std::unordered_map<std::uint32_t, std::unique_ptr<unlit_material>> m_materials;

    std::uint32_t m_max_lod{0};
    std::uint32_t m_bvh_node_count{0};

    std::unique_ptr<geometry> m_debug_geometry;
    std::unique_ptr<unlit_material> m_debug_material;
    std::vector<entity> m_debug_entities;

    std::uint32_t m_check_times{0};

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