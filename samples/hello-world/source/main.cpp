#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/renderers/features/bloom_render_feature.hpp"
#include "graphics/renderers/features/gtao_render_feature.hpp"
#include "graphics/renderers/features/shadow_render_feature.hpp"
#include "graphics/renderers/features/taa_render_feature.hpp"
#include "graphics/renderers/features/vsm_render_feature.hpp"
#include "sample/sample_system.hpp"
#include <imgui.h>

namespace violet
{
class hello_world : public sample_system
{
public:
    hello_world()
        : sample_system("hello_world")
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

        m_box_geometry = std::make_unique<box_geometry>();

        m_pbr_material = std::make_unique<pbr_material>();
        m_pbr_material->set_metallic(0.5f);
        m_pbr_material->set_roughness(0.5f);

        m_unlit_material = std::make_unique<unlit_material>();
        m_unlit_material->set_color({1.0f, 1.0f, 1.0f});

        // Plane.
        m_plane = world.create();
        world.add_component<transform_component, mesh_component, scene_component>(m_plane);

        auto& plane_mesh = world.get_component<mesh_component>(m_plane);
        plane_mesh.geometry = m_box_geometry.get();
        plane_mesh.flags |= MESH_STATIC;
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
            box_mesh.flags |= MESH_STATIC;

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

        if (ImGui::CollapsingHeader("Light"))
        {
            static float rotate_x = 0.0f;
            static float rotate_y = 0.0f;

            bool transform_dirty = false;
            bool ligth_dirty = false;

            static float color[3] = {1.0f, 1.0f, 1.0f};
            static float intensity = 7.0f;

            if (ImGui::ColorEdit3("Color", color))
            {
                ligth_dirty = true;
            }

            if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 10.0f))
            {
                ligth_dirty = true;
            }

            if (ImGui::SliderFloat("Rotate X", &rotate_x, 0.0, 360.0))
            {
                transform_dirty = true;
            }

            if (ImGui::SliderFloat("Rotate Y", &rotate_y, 0.0, 360.0))
            {
                transform_dirty = true;
            }

            if (transform_dirty)
            {
                auto& transform = world.get_component<transform_component>(get_light());
                transform.set_rotation(
                    quaternion::from_euler(
                        vec3f{math::to_radians(rotate_x), math::to_radians(rotate_y), 0.0f}));
            }

            if (ligth_dirty)
            {
                auto& light = world.get_component<light_component>(get_light());
                light.color = {.x = color[0], .y = color[1], .z = color[2]};
                light.color *= intensity;
            }
        }

        if (ImGui::CollapsingHeader("Shadow"))
        {
            auto& main_camera = get_world().get_component<camera_component>(get_camera());
            auto* renderer = static_cast<deferred_renderer*>(main_camera.renderer.get());

            auto* shadow = renderer->get_feature<shadow_render_feature>();

            static auto sample_mode = static_cast<std::int32_t>(shadow->get_sample_mode());
            static const char* sample_mode_items[] = {
                "None",
                "PCF",
                // "PCSS",
            };

            if (ImGui::Combo(
                    "Sample Mode",
                    &sample_mode,
                    sample_mode_items,
                    IM_ARRAYSIZE(sample_mode_items)))
            {
                shadow->set_sample_mode(static_cast<shadow_sample_mode>(sample_mode));
            }

            static float sample_radius = shadow->get_sample_radius() * 100.0f;
            if (ImGui::SliderFloat("Sample Radius", &sample_radius, 0.0f, 10.0f))
            {
                shadow->set_sample_radius(sample_radius * 0.01f);
            }

            static float normal_offset = shadow->get_normal_offset();
            if (ImGui::SliderFloat("Normal Offset", &normal_offset, 0.0f, 20.0f))
            {
                shadow->set_normal_offset(normal_offset);
            }

            static float constant_bias = shadow->get_constant_bias();
            if (ImGui::SliderFloat("Constant Bias", &constant_bias, 0.0f, 0.1f))
            {
                shadow->set_constant_bias(constant_bias);
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

        if (ImGui::CollapsingHeader("Bloom"))
        {
            auto& main_camera = get_world().get_component<camera_component>(get_camera());
            auto* bloom = main_camera.renderer->get_feature<bloom_render_feature>();

            static bool enable_bloom = bloom->is_enable();
            static float threshold = bloom->get_threshold();
            static float intensity = bloom->get_intensity();

            ImGui::Checkbox("Enable##Bloom", &enable_bloom);
            ImGui::SliderFloat("Threshold##Bloom", &threshold, 0.0f, 2.0f);
            ImGui::SliderFloat("Intensity##Bloom", &intensity, 0.0f, 1.0f);

            if (enable_bloom)
            {
                bloom->enable();
            }
            else
            {
                bloom->disable();
            }
            bloom->set_threshold(threshold);
            bloom->set_intensity(intensity);
        }

        if (ImGui::CollapsingHeader("Debug"))
        {
            auto& main_camera = get_world().get_component<camera_component>(get_camera());
            auto* renderer = static_cast<deferred_renderer*>(main_camera.renderer.get());

            static int debug_mode = static_cast<int>(renderer->get_debug_mode());
            const char* debug_mode_items[] = {
                "None",
                "VSM Page",
                "VSM Page Cache",
                "Shadow Mask",
            };

            if (ImGui::Combo(
                    "Debug Mode",
                    &debug_mode,
                    debug_mode_items,
                    IM_ARRAYSIZE(debug_mode_items)))
            {
                renderer->set_debug_mode(static_cast<deferred_renderer::debug_mode>(debug_mode));
            }

            static int debug_info = 0;
            const char* debug_info_items[] = {
                "None",
                "VSM",
            };

            if (ImGui::Combo(
                    "Debug Info",
                    &debug_info,
                    debug_info_items,
                    IM_ARRAYSIZE(debug_info_items)))
            {
                auto* vsm = renderer->get_feature<vsm_render_feature>();
                vsm->set_debug_info(debug_info == 1);
            }

            if (debug_info == 1)
            {
                auto* vsm = renderer->get_feature<vsm_render_feature>();
                auto debug_info = vsm->get_debug_info();

                ImGui::Text("Cache Hit: %d", debug_info.cache_hit);
                ImGui::Text("Cache Miss: %d", debug_info.rendered);
                ImGui::Text("Unmapped: %d", debug_info.unmapped);
                ImGui::Text("Static Drawcall: %d", debug_info.static_drawcall);
                ImGui::Text("Dynamic Drawcall: %d", debug_info.dynamic_drawcall);
            }
        }
    }

    entity m_root;

    std::unique_ptr<geometry> m_box_geometry;

    std::unique_ptr<pbr_material> m_pbr_material;
    std::unique_ptr<unlit_material> m_unlit_material;

    entity m_plane;
    std::array<entity, 2> m_boxes;
};
} // namespace violet

int main()
{
    violet::application app("assets/config/hello-world.json");
    app.install<violet::hello_world>();
    app.run();

    return 0;
}