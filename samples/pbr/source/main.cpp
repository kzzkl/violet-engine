#include "components/camera_component.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/features/gtao_render_feature.hpp"
#include "graphics/renderers/features/taa_render_feature.hpp"
#include "sample/sample_system.hpp"
#include <imgui.h>

namespace violet
{
class pbr_sample : public sample_system
{
public:
    pbr_sample()
        : sample_system("pbr_sample")
    {
    }

    bool initialize(const dictionary& config) override
    {
        if (!sample_system::initialize(config))
        {
            return false;
        }

        m_root = load_model(config["model"]);

        auto& world = get_world();

        // Plane.
        m_plane = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_plane);

        m_box_geometry = std::make_unique<box_geometry>();

        m_pbr_material = std::make_unique<pbr_material>();
        m_pbr_material->set_metallic(0.5f);
        m_pbr_material->set_roughness(0.5f);

        m_unlit_material = std::make_unique<unlit_material>();
        m_unlit_material->set_color({1.0f, 1.0f, 1.0f});

        auto& plane_mesh = world.get_component<mesh_component>(m_plane);
        plane_mesh.geometry = m_box_geometry.get();
        plane_mesh.submeshes.push_back({
            .index = 0,
            .material = m_pbr_material.get(),
        });
        auto& plane_transform = world.get_component<transform_component>(m_plane);
        plane_transform.set_position({0.0f, -1.0f, 0.0f});
        plane_transform.set_scale({10.0f, 0.05f, 10.0f});

        for (std::uint32_t i = 0; i < m_boxes.size(); ++i)
        {
            m_boxes[i] = world.create();
            world.add_component<transform_component, mesh_component, scene_component>(m_boxes[i]);

            auto& box_mesh = world.get_component<mesh_component>(m_boxes[i]);
            box_mesh.geometry = m_box_geometry.get();

            if (i % 2 == 0)
            {
                box_mesh.submeshes.push_back({
                    .material = m_unlit_material.get(),
                });
            }
            else
            {
                box_mesh.submeshes.push_back({
                    .material = m_pbr_material.get(),
                });
            }

            auto& box_transform = world.get_component<transform_component>(m_boxes[i]);
            box_transform.set_position({2.0f * static_cast<float>(i), 3.0f, 0.0f});
        }

        return true;
    }

private:
    void tick() override
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
            static float metallic = m_pbr_material->get_metallic();
            static float roughness = m_pbr_material->get_roughness();
            static float albedo[] = {1.0f, 1.0f, 1.0f};

            if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f))
            {
                m_pbr_material->set_metallic(metallic);
            }

            if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f))
            {
                m_pbr_material->set_roughness(roughness);
            }

            if (ImGui::ColorEdit3("Albedo", albedo))
            {
                m_pbr_material->set_albedo({albedo[0], albedo[1], albedo[2]});
            }
        }

        if (ImGui::CollapsingHeader("TAA"))
        {
            auto& main_camera = get_world().get_component<camera_component>(get_camera());
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
            auto& main_camera = get_world().get_component<camera_component>(get_camera());
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

    entity m_root;

    std::unique_ptr<geometry> m_box_geometry;

    std::unique_ptr<pbr_material> m_pbr_material;
    std::unique_ptr<unlit_material> m_unlit_material;

    entity m_plane;
    std::array<entity, 4> m_boxes;
};
} // namespace violet

int main()
{
    violet::application app("assets/config/pbr.json");
    app.install<violet::pbr_sample>();
    app.run();

    return 0;
}