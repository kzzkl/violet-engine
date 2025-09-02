#include "cluster_material.hpp"
#include "common/log.hpp"
#include "components/camera_component.hpp"
#include "components/hierarchy_component.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include "sample/gltf_loader.hpp"
#include "sample/sample_system.hpp"
#include <imgui.h>

namespace violet
{
class cluster_sample : public sample_system
{
public:
    cluster_sample()
        : sample_system("cluster demo")
    {
    }

    bool initialize(const dictionary& config) override
    {
        if (!sample_system::initialize(config))
        {
            return false;
        }

        if (config.find("cpu_culling") != config.end())
        {
            m_cpu_culling = config["cpu_culling"];
        }

        m_material = std::make_unique<cluster_material>();

        if (config.contains("model"))
        {
            load_model(config["model"]);
        }

        return true;
    }

private:
    void tick() override
    {
        if (!m_cpu_culling)
        {
            return;
        }

        const char* cull_modes[] = {"Triangle", "Cluster", "Cluster Node", "Material"};
        static int mode = 3;

        ImGui::Combo("Mode", &mode, cull_modes, IM_ARRAYSIZE(cull_modes));

        static bool auto_cull = true;
        ImGui::Checkbox("Cull", &auto_cull);

        if (auto_cull)
        {
            std::uint32_t triangle_count = cull(mode);
            ImGui::Text("Triangle: %u", triangle_count);
        }
        else
        {
            static int lod = 0;
            ImGui::SliderInt(
                "LOD",
                &lod,
                0,
                static_cast<int>(m_submeshes[0].cluster_nodes[0].child_count - 1));

            static bool show_cluster = false;
            ImGui::Checkbox("Show Cluster", &show_cluster);

            std::vector<std::uint32_t> cluster_indexes;

            std::queue<std::uint32_t> queue;
            queue.push(lod + 1);

            while (!queue.empty())
            {
                const auto& cluster_node = m_submeshes[0].cluster_nodes[queue.front()];
                queue.pop();

                if (cluster_node.is_leaf)
                {
                    for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
                    {
                        cluster_indexes.push_back(cluster_node.child_offset + i);
                    }
                }
                else
                {
                    for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
                    {
                        queue.push(cluster_node.child_offset + i);
                    }
                }
            }

            auto& mesh = get_world().get_component<mesh_component>(m_mesh);
            mesh.submeshes.clear();
            if (show_cluster)
            {
                static int cluster = 0;
                ImGui::SliderInt(
                    "Cluster",
                    &cluster,
                    0,
                    static_cast<int>(cluster_indexes.size() - 1));

                mesh.submeshes.push_back({
                    .index = cluster_indexes[cluster],
                    .material = m_models[0].materials[0].get(),
                });
            }
            else
            {
                for (std::uint32_t cluster_indexe : cluster_indexes)
                {
                    mesh.submeshes.push_back({
                        .index = cluster_indexe,
                        .material = m_models[0].materials[0].get(),
                    });
                }
            }
        }
    }

    void load_model(std::string_view path)
    {
        std::string name(path.substr(path.find_last_of('/') + 1));

        auto& world = get_world();

        gltf_loader loader;

        auto result = loader.load(path);
        if (!result)
        {
            return;
        }

        model model = {};
        model.textures = std::move(result->textures);

        for (std::size_t i = 0; i < result->geometries.size(); ++i)
        {
            const auto& geometry_data = result->geometries[i];

            auto model_geometry = std::make_unique<geometry>();
            model_geometry->set_positions(geometry_data.positions);
            model_geometry->set_normals(geometry_data.normals);
            model_geometry->set_tangents(geometry_data.tangents);
            model_geometry->set_texcoords(geometry_data.texcoords);
            model_geometry->set_indexes(geometry_data.indexes);

            geometry_tool::cluster_input input = {
                .positions = geometry_data.positions,
                .normals = geometry_data.normals,
                .tangents = geometry_data.tangents,
                .texcoords = geometry_data.texcoords,
                .indexes = geometry_data.indexes,
            };

            for (const auto& submesh_data : geometry_data.submeshes)
            {
                input.submeshes.push_back({
                    .vertex_offset = submesh_data.vertex_offset,
                    .index_offset = submesh_data.index_offset,
                    .index_count = submesh_data.index_count,
                });
            }

            // auto output = geometry_tool::generate_clusters(input);
            // output.save(name + ".cluster_" + std::to_string(i));

            geometry_tool::cluster_output output;
            output.load(name + ".cluster_" + std::to_string(i));

            model_geometry->set_positions(output.positions);
            model_geometry->set_normals(output.normals);
            model_geometry->set_tangents(output.tangents);
            model_geometry->set_texcoords(output.texcoords);
            model_geometry->set_indexes(output.indexes);

            if (m_cpu_culling)
            {
                for (const auto& submesh : output.submeshes)
                {
                    m_submeshes.push_back({
                        .offset =
                            static_cast<std::uint32_t>(model_geometry->get_submeshes().size()),
                        .clusters = submesh.clusters,
                        .cluster_nodes = submesh.cluster_nodes,
                    });

                    for (const auto& cluster : submesh.clusters)
                    {
                        model_geometry->add_submesh(0, cluster.index_offset, cluster.index_count);
                    }
                }
            }
            else
            {
                for (const auto& submesh : output.submeshes)
                {
                    model_geometry->add_submesh(submesh.clusters, submesh.cluster_nodes);
                }
            }

            model.geometries.push_back(std::move(model_geometry));

            log::info("{} / {}", model.geometries.size(), result->geometries.size());
        }

        for (const auto& material_data : result->materials)
        {
            auto model_material = std::make_unique<pbr_material>();
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
                model_material->set_roughness_metallic(material_data.roughness_metallic_texture);
            }

            if (material_data.emissive_texture != nullptr)
            {
                model_material->set_emissive(material_data.emissive_texture);
            }

            if (material_data.normal_texture != nullptr)
            {
                model_material->set_normal(material_data.normal_texture);
            }

            model.materials.push_back(std::move(model_material));
        }

        for (const auto& node_data : result->nodes)
        {
            entity entity = world.create();
            world.add_component<transform_component, scene_component>(entity);

            auto& transform = world.get_component<transform_component>(entity);
            transform.set_position(node_data.position);
            transform.set_rotation(node_data.rotation);
            transform.set_scale(node_data.scale);

            model.entities.push_back(entity);
        }

        for (std::size_t i = 0; i < result->nodes.size(); ++i)
        {
            const auto& node = result->nodes[i];
            const auto entity = model.entities[i];

            if (node.mesh != -1)
            {
                world.add_component<mesh_component>(entity);

                const auto& mesh_data = result->meshes[node.mesh];

                auto& entity_mesh = world.get_component<mesh_component>(entity);
                entity_mesh.geometry = model.geometries[mesh_data.geometry].get();

                for (std::size_t j = 0; j < mesh_data.submeshes.size(); ++j)
                {
                    assert(mesh_data.submeshes[i] < entity_mesh.geometry->get_submeshes().size());

                    entity_mesh.submeshes.push_back({
                        .index = mesh_data.submeshes[j],
                        .material = model.materials[mesh_data.materials[j]].get(),
                        // .material = m_material.get(),
                    });
                }

                m_mesh = entity;
            }

            if (node.parent != -1)
            {
                world.add_component<parent_component>(entity);
                world.get_component<parent_component>(entity).parent = model.entities[node.parent];
            }
        }

        m_models.push_back(std::move(model));

        // auto add_clone = [&](const vec3f& position)
        // {
        //     entity entity = world.create();
        //     world.add_component<transform_component, scene_component, mesh_component>(entity);

        //     auto& transform = world.get_component<transform_component>(entity);
        //     transform.set_position(position);

        //     auto& mesh = world.get_component<mesh_component>(entity);
        //     mesh = world.get_component<mesh_component>(m_mesh);
        // };

        // const float size = 50.0f;
        // for (float i = 0; i < size; ++i)
        // {
        //     for (float j = 0; j < size; ++j)
        //     {
        //         for (float k = 0; k < size; ++k)
        //         {
        //             add_clone(vec3f(
        //                 (i - (size * 0.5f)) * 2.0f,
        //                 (j - (size * 0.5f)) * 2.0f,
        //                 (k - (size * 0.5f)) * 2.0f));
        //         }
        //     }
        // }
    }

    std::uint32_t cull(std::uint32_t mode, float threshold = 1.0f)
    {
        auto& world = get_world();

        const auto& camera = world.get_component<const camera_component>(get_camera());

        float aspect = static_cast<float>(camera.get_extent().width) /
                       static_cast<float>(camera.get_extent().height);

        mat4f matrix_p =
            camera.far == std::numeric_limits<float>::infinity() ?
                matrix::perspective_reverse_z(camera.fov, aspect, camera.near) :
                matrix::perspective_reverse_z(camera.fov, aspect, camera.near, camera.far);

        const auto& camera_world =
            world.get_component<const transform_world_component>(get_camera());
        mat4f matrix_v = matrix::inverse_transform(camera_world.matrix);

        const auto& mesh_world = world.get_component<const transform_world_component>(m_mesh);
        mat4f matrix_mv = matrix::mul(mesh_world.matrix, matrix_v);

        float lod_scale =
            static_cast<float>(camera.get_extent().height) * 0.5f / std::tan(camera.fov * 0.5f);

        auto check_cluster_node_lod = [&](cluster_node cluster_node)
        {
            vec3f center = matrix::mul(
                vec4f{
                    cluster_node.lod_bounds.center.x,
                    cluster_node.lod_bounds.center.y,
                    cluster_node.lod_bounds.center.z,
                    1.0f},
                matrix_mv);
            float radius = cluster_node.lod_bounds.radius * vector::max(mesh_world.scale);

            float near = center.z - radius;
            float far = center.z + radius;

            if (near < camera.near)
            {
                return true;
            }

            float scale = lod_scale * vector::max(mesh_world.scale);

            float min_error = scale * cluster_node.min_lod_error / far;
            float max_error = scale * cluster_node.max_parent_lod_error / near;

            return min_error <= threshold && threshold < max_error;
        };

        auto check_cluster_lod = [&](cluster cluster)
        {
            vec3f center = matrix::mul(
                vec4f{
                    cluster.lod_bounds.center.x,
                    cluster.lod_bounds.center.y,
                    cluster.lod_bounds.center.z,
                    1.0f},
                matrix_mv);
            float radius = cluster.lod_bounds.radius * vector::max(mesh_world.scale);

            float near = center.z - radius;

            // if camera inside lod sphere, use lod 0.
            if (near < camera.near)
            {
                return cluster.lod_error == -1.0;
            }

            float lod_error = lod_scale * cluster.lod_error * vector::max(mesh_world.scale) / near;
            return lod_error <= threshold;
        };

        std::uint32_t triangle_count = 0;

        auto& mesh = world.get_component<mesh_component>(m_mesh);
        mesh.submeshes.clear();

        for (const auto& submesh : m_submeshes)
        {
            std::queue<std::uint32_t> queue;
            queue.push(0);

            while (!queue.empty())
            {
                std::uint32_t cluster_node_index = queue.front();
                queue.pop();

                const auto& cluster_node = submesh.cluster_nodes[cluster_node_index];

                if (!check_cluster_node_lod(cluster_node))
                {
                    continue;
                }

                if (cluster_node.is_leaf)
                {
                    for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
                    {
                        if (!check_cluster_lod(submesh.clusters[cluster_node.child_offset + i]))
                        {
                            continue;
                        }

                        material* material = nullptr;
                        switch (mode)
                        {
                        case 0:
                            material = m_material.get();
                            break;
                        case 1:
                            material = get_material(submesh.offset + cluster_node.child_offset + i);
                            break;
                        case 2:
                            material = get_material(cluster_node_index);
                            break;
                        case 3:
                            material = m_models[0].materials[0].get();
                            break;
                        default:
                            break;
                        }

                        mesh.submeshes.push_back({
                            .index = submesh.offset + cluster_node.child_offset + i,
                            .material = material,
                        });

                        triangle_count +=
                            submesh.clusters[cluster_node.child_offset + i].index_count / 3;
                    }
                }
                else
                {
                    for (std::uint32_t i = 0; i < cluster_node.child_count; ++i)
                    {
                        queue.push(cluster_node.child_offset + i);
                    }
                }
            }
        }

        return triangle_count;
    }

    material* get_material(std::uint32_t index)
    {
        auto iter = m_materials.find(index);
        if (iter != m_materials.end())
        {
            return iter->second.get();
        }

        std::uint32_t hash = index + 1;
        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;

        vec3f color(
            static_cast<float>((hash >> 0) & 255) / 255.0f,
            static_cast<float>((hash >> 8) & 255) / 255.0f,
            static_cast<float>((hash >> 16) & 255) / 255.0f);

        // auto material = std::make_unique<unlit_material>();
        auto material = std::make_unique<pbr_material>();
        material->set_albedo(color);
        m_materials[index] = std::move(material);
        return m_materials[index].get();
    }

    entity m_mesh;

    struct model
    {
        std::vector<std::unique_ptr<geometry>> geometries;
        std::vector<std::unique_ptr<material>> materials;
        std::vector<std::unique_ptr<texture_2d>> textures;
        std::vector<entity> entities;
    };

    std::vector<model> m_models;
    std::unique_ptr<material> m_material;

    bool m_cpu_culling{false};

    struct submesh
    {
        std::uint32_t offset;
        std::vector<cluster> clusters;
        std::vector<cluster_node> cluster_nodes;
    };
    std::vector<submesh> m_submeshes;
    std::unordered_map<std::uint32_t, std::unique_ptr<material>> m_materials;
};
} // namespace violet

int main()
{
    violet::application app("assets/config/cluster.json");
    app.install<violet::cluster_sample>();
    app.run();

    return 0;
}