#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/skybox_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/geometries/plane_geometry.hpp"
#include "graphics/geometries/sphere_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/features/taa_feature.hpp"
#include "graphics/skybox.hpp"
#include "graphics/tools/geometry_tool.hpp"
#include "sample/deferred_renderer_imgui.hpp"
#include "sample/gltf_loader.hpp"
#include "sample/imgui_system.hpp"
#include "sample/sample_system.hpp"
#include "window/window_system.hpp"
#include <imgui.h>

namespace violet
{
class mesh_simplifier_demo : public sample_system
{
public:
    mesh_simplifier_demo()
        : sample_system("mesh_simplifier_demo")
    {
    }

    bool initialize(const dictionary& config) override
    {
        if (!sample_system::initialize(config))
        {
            return false;
        }

        initialize_scene(config["model"], config["skybox"]);

        return true;
    }

private:
    void initialize_scene(std::string_view model_path, std::string_view skybox_path)
    {
        auto& world = get_world();

        // Model.
        m_line_material = std::make_unique<unlit_material>(
            RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            RHI_CULL_MODE_NONE,
            RHI_POLYGON_MODE_LINE);
        m_material = std::make_unique<pbr_material>();

        if (!model_path.empty())
        {
            gltf_loader loader;
            if (auto result = loader.load(model_path))
            {
                const auto& geometry_data = result->geometries[0];

                m_original_geometry = std::make_unique<geometry>();
                m_original_geometry->set_positions(geometry_data.positions);
                m_original_geometry->set_normals(geometry_data.normals);
                m_original_geometry->set_tangents(geometry_data.tangents);
                m_original_geometry->set_texcoords(geometry_data.texcoords);
                m_original_geometry->set_indexes(geometry_data.indexes);
                for (const auto& submesh : geometry_data.submeshes)
                {
                    m_original_geometry->add_submesh(
                        submesh.vertex_offset,
                        submesh.index_offset,
                        submesh.index_count);
                }

                if (!result->materials.empty())
                {
                    const auto& material_data = result->materials[0];

                    auto material = std::make_unique<pbr_material>();
                    material->set_albedo(material_data.albedo);
                    material->set_roughness(material_data.roughness);
                    material->set_metallic(material_data.metallic);
                    material->set_emissive(material_data.emissive);

                    if (material_data.albedo_texture != nullptr)
                    {
                        material->set_albedo(material_data.albedo_texture);
                    }

                    if (material_data.roughness_metallic_texture != nullptr)
                    {
                        material->set_roughness_metallic(material_data.roughness_metallic_texture);
                    }

                    if (material_data.emissive_texture != nullptr)
                    {
                        material->set_emissive(material_data.emissive_texture);
                    }

                    if (material_data.normal_texture != nullptr)
                    {
                        material->set_normal(material_data.normal_texture);
                    }

                    m_material = std::move(material);

                    m_textures = std::move(result->textures);
                }
            }
            else
            {
                m_original_geometry = std::make_unique<sphere_geometry>(0.5f);
            }
        }
        else
        {
            // m_original_geometry = std::make_unique<sphere_geometry>(0.5f, 32, 16);
            // m_original_geometry = std::make_unique<box_geometry>(0.5f, 0.5f, 0.5f, 3, 3, 3);
            m_original_geometry = std::make_unique<plane_geometry>(1.0f, 1.0f, 2, 2);
        }

        m_model = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_model);

        auto& mesh = world.get_component<mesh_component>(m_model);
        mesh.geometry = m_original_geometry.get();
        mesh.submeshes.push_back({
            .index = 0,
            .material = m_material.get(),
        });

        m_simplified_geometry = std::make_unique<geometry>();
        m_simplified_geometry->set_positions(m_original_geometry->get_positions());
        m_simplified_geometry->set_normals(m_original_geometry->get_normals());
        m_simplified_geometry->set_tangents(m_original_geometry->get_tangents());
        m_simplified_geometry->set_texcoords(m_original_geometry->get_texcoords());
        m_simplified_geometry->set_indexes(m_original_geometry->get_indexes());
        m_simplified_geometry->add_submesh(0, 0, m_original_geometry->get_indexes().size());

        m_edge_material = std::make_unique<unlit_material>(
            RHI_PRIMITIVE_TOPOLOGY_LINE_LIST,
            RHI_CULL_MODE_NONE,
            RHI_POLYGON_MODE_LINE);
        m_edge_material->set_color({1.0f, 0.0f, 0.0f});

        m_edge_geometry = std::make_unique<geometry>();
        m_edge_geometry->add_submesh(0, 0, 0);

        auto edge_entity = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(edge_entity);

        auto& edge_mesh = world.get_component<mesh_component>(edge_entity);
        edge_mesh.geometry = m_edge_geometry.get();
        edge_mesh.submeshes.push_back({
            .index = 0,
            .material = m_edge_material.get(),
        });
    }

    void tick() override
    {
        auto& world = get_world();

        static bool show_simplified = false;
        if (ImGui::Checkbox("Simplified", &show_simplified))
        {
            auto& mesh = world.get_component<mesh_component>(m_model);
            mesh.geometry =
                show_simplified ? m_simplified_geometry.get() : m_original_geometry.get();
        }

        static float simplify_ratio = 1.0f;
        if (ImGui::SliderFloat("Simplify Ratio", &simplify_ratio, 0.0f, 1.0f))
        {
            std::uint32_t triangle_count = m_original_geometry->get_index_count() / 3;
            auto target_triangle_count =
                static_cast<std::uint32_t>(static_cast<float>(triangle_count) * simplify_ratio);

            std::vector<vec3f> positions(
                m_original_geometry->get_positions().begin(),
                m_original_geometry->get_positions().end());
            std::vector<vec3f> normals(
                m_original_geometry->get_normals().begin(),
                m_original_geometry->get_normals().end());
            std::vector<vec4f> tangents(
                m_original_geometry->get_tangents().begin(),
                m_original_geometry->get_tangents().end());
            std::vector<vec2f> texcoords(
                m_original_geometry->get_texcoords().begin(),
                m_original_geometry->get_texcoords().end());
            std::vector<std::uint32_t> indexes(
                m_original_geometry->get_indexes().begin(),
                m_original_geometry->get_indexes().end());

            auto output = geometry_tool::simplify({
                .positions = positions,
                .normals = normals,
                .tangents = tangents,
                .texcoords = texcoords,
                .indexes = indexes,
                .target_triangle_count = target_triangle_count,
            });

            positions.resize(output.vertex_count);
            normals.resize(output.vertex_count);
            indexes.resize(output.index_count);

            m_simplified_geometry->set_positions(positions);
            m_simplified_geometry->set_normals(normals);
            m_simplified_geometry->set_tangents(tangents);
            m_simplified_geometry->set_texcoords(texcoords);
            m_simplified_geometry->set_indexes(indexes);
            m_simplified_geometry->clear_submeshes();
            m_simplified_geometry->add_submesh(0, 0, indexes.size());

            std::vector<std::uint32_t> edge_indexes(output.edge_vertices.size());
            std::iota(edge_indexes.begin(), edge_indexes.end(), 0);
            m_edge_geometry->set_positions(output.edge_vertices);
            m_edge_geometry->set_indexes(edge_indexes);
            m_edge_geometry->clear_submeshes();
            m_edge_geometry->add_submesh(0, 0, edge_indexes.size());
        }

        static bool show_edge = false;
        if (ImGui::Checkbox("Edge", &show_edge))
        {
            auto& mesh = world.get_component<mesh_component>(m_model);
            if (show_edge)
            {
                mesh.submeshes.clear();
            }
            else
            {
                mesh.submeshes.push_back({
                    .index = 0,
                    .material = m_material.get(),
                });
            }
        }

        static bool show_line = false;
        if (ImGui::Checkbox("Line", &show_line))
        {
            auto& mesh = world.get_component<mesh_component>(m_model);
            if (show_line)
            {
                mesh.submeshes[0].material = m_line_material.get();
            }
            else
            {
                mesh.submeshes[0].material = m_material.get();
            }
        }
    }

    entity m_model;

    std::unique_ptr<material> m_material;
    std::unique_ptr<geometry> m_original_geometry;
    std::unique_ptr<geometry> m_simplified_geometry;

    std::unique_ptr<unlit_material> m_edge_material;
    std::unique_ptr<geometry> m_edge_geometry;

    std::unique_ptr<unlit_material> m_line_material;

    std::vector<std::unique_ptr<texture_2d>> m_textures;
};
} // namespace violet

int main()
{
    violet::application app("assets/config/mesh-simplifier.json");
    app.install<violet::mesh_simplifier_demo>();
    app.run();

    return 0;
}