#include "components/camera_component.hpp"
#include "components/first_person_control_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/materials/pbr_material.hpp"
#include "graphics/materials/unlit_material.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/renderers/features/atmosphere_feature.hpp"
#include "graphics/renderers/features/bloom_feature.hpp"
#include "graphics/renderers/features/eye_adaptation_feature.hpp"
#include "graphics/renderers/features/gtao_feature.hpp"
#include "graphics/renderers/features/shadow_feature.hpp"
#include "graphics/renderers/features/taa_feature.hpp"
#include "graphics/renderers/features/vsm_feature.hpp"
#include "math/euler.hpp"
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

        if (config.contains("model"))
        {
            load_options options = 0;
            options |= LOAD_OPTION_GENERATE_CLUSTERS;
            options |= LOAD_OPTION_GENERATE_MIPMAPS;
            options |= LOAD_OPTION_COMPRESS_TEXTURES;

            m_root = load_model(config["model"], options);
        }

        auto& world = get_world();

        auto& main_camera = world.get_component<camera_component>(get_camera());
        main_camera.renderer->set_profiling(true);

        m_box_geometry = std::make_unique<box_geometry>();

        m_pbr_material = std::make_unique<pbr_material>();
        m_pbr_material->set_metallic(0.5f);
        m_pbr_material->set_roughness(0.5f);

        m_unlit_material = std::make_unique<unlit_material>();
        m_unlit_material->set_color({1.0f, 1.0f, 1.0f});

        // Plane.
        // entity plane = world.create();
        // world.add_component<transform_component, mesh_component, scene_component>(plane);

        // auto& plane_mesh = world.get_component<mesh_component>(plane);
        // plane_mesh.geometry = m_box_geometry.get();
        // plane_mesh.flags |= MESH_STATIC;
        // plane_mesh.submeshes.push_back({
        //     .index = 0,
        //     .material = m_pbr_material.get(),
        // });
        // auto& plane_transform = world.get_component<transform_component>(plane);
        // plane_transform.set_position({0.0f, -1.0f, 0.0f});
        // plane_transform.set_scale({10.0f, 0.05f, 10.0f});

        // for (std::uint32_t i = 0; i < 50; ++i)
        // {
        //     for (std::uint32_t j = 0; j < 50; ++j)
        //     {
        //         entity box = world.create();
        //         world.add_component<transform_component, mesh_component, scene_component>(box);

        //         auto& box_mesh = world.get_component<mesh_component>(box);
        //         box_mesh.geometry = m_box_geometry.get();
        //         box_mesh.flags |= MESH_STATIC;
        //         box_mesh.submeshes.push_back({
        //             .material = m_pbr_material.get(),
        //         });

        //         vec3f position = {
        //             static_cast<float>(i) * 2.0f,
        //             2.0f,
        //             static_cast<float>(j) * 2.0f,
        //         };

        //         auto& box_transform = world.get_component<transform_component>(box);
        //         box_transform.set_position(position);
        //     }
        // }

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
            if (ImGui::SliderFloat("Translate", &translate, -50000.0f, 5.0))
            {
                auto& transform = world.get_component<transform_component>(m_root);
                transform.set_position({0.0f, translate, 0.0f});
            }

            static float scale = 1.0f;
            if (ImGui::SliderFloat("Scale", &scale, 1.0f, 5000.0f))
            {
                auto& transform = world.get_component<transform_component>(m_root);
                transform.set_scale({scale, scale, scale});
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

        if (ImGui::CollapsingHeader("Camera"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto& controller = world.get_component<first_person_control_component>(get_camera());
            ImGui::SliderFloat("Move Speed", &controller.move_speed, 0.0f, 5000.0f);

            const char* camera_types[] = {"Perspective", "Orthographic"};
            static int camera_type = static_cast<int>(main_camera.type);
            if (ImGui::Combo("Camera Type", &camera_type, camera_types, IM_ARRAYSIZE(camera_types)))
            {
                if (camera_type == CAMERA_PERSPECTIVE)
                {
                    main_camera.type = CAMERA_PERSPECTIVE;
                    main_camera.near = 0.01f;
                    main_camera.far = std::numeric_limits<float>::infinity();
                }
                else
                {
                    main_camera.type = CAMERA_ORTHOGRAPHIC;
                    main_camera.near = 0.0f;
                    main_camera.far = 1000.0f;
                }
            }

            if (camera_type == CAMERA_PERSPECTIVE)
            {
                static float fov = math::to_degrees(main_camera.perspective.fov);
                ImGui::SliderFloat("FOV", &fov, 1.0f, 120.0f);
                main_camera.perspective.fov = math::to_radians(fov);
            }
            else if (camera_type == CAMERA_ORTHOGRAPHIC)
            {
                ImGui::SliderFloat(
                    "Orthographic Size",
                    &main_camera.orthographic.size,
                    1.0f,
                    100.0f);
            }

            const auto& camera_transform =
                world.get_component<const transform_component>(get_camera());
            vec3f camera_position = camera_transform.get_position();
            ImGui::Text(
                "x: %f, y: %f, z: %f",
                camera_position.x,
                camera_position.y,
                camera_position.z);
        }

        if (ImGui::CollapsingHeader("Sky"))
        {
            bool transform_dirty = false;
            bool ligth_dirty = false;
            bool skybox_dirty = false;

            static float color[3] = {1.0f, 1.0f, 1.0f};
            if (ImGui::ColorEdit3("Sun Color", color))
            {
                ligth_dirty = true;
            }

            static float intensity = 10.0f;
            if (ImGui::SliderFloat("Sun Intensity", &intensity, 0.0f, 30.0f))
            {
                ligth_dirty = true;
            }

            if (ligth_dirty)
            {
                auto& light = world.get_component<light_component>(get_sky());
                light.color = {.x = color[0], .y = color[1], .z = color[2]};
                light.color *= intensity;
            }

            static vec3f euler = euler::from_quaternion(
                world.get_component<const transform_component>(get_sky()).get_rotation());

            if (ImGui::SliderFloat("Sun Rotate X", &euler.x, -math::HALF_PI, math::HALF_PI))
            {
                transform_dirty = true;
            }

            if (ImGui::SliderFloat("Sun Rotate Y", &euler.y, 0.0, math::TWO_PI))
            {
                transform_dirty = true;
            }

            if (transform_dirty)
            {
                auto& transform = world.get_component<transform_component>(get_sky());
                transform.set_rotation(quaternion::from_euler(euler));
            }

            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto* atmosphere = main_camera.renderer->get_feature<atmosphere_feature>();

            static bool use_multi_scattering = atmosphere->get_use_multi_scattering();
            if (ImGui::Checkbox("Multi Scattering", &use_multi_scattering))
            {
                atmosphere->set_use_multi_scattering(use_multi_scattering);
            }
        }

        if (ImGui::CollapsingHeader("Shadow"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto* shadow = main_camera.renderer->get_feature<shadow_feature>();

            static auto sample_mode = static_cast<std::int32_t>(shadow->sample_mode);
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
                shadow->sample_mode = static_cast<shadow_sample_mode>(sample_mode);
            }

            ImGui::SliderFloat("Sample Radius", &shadow->sample_radius, 0.0f, 10.0f);
            ImGui::SliderFloat(
                "Slope Scale Depth Bias",
                &shadow->slope_scale_depth_bias,
                0.0f,
                2.0f);
            ImGui::SliderFloat("Normal Bias", &shadow->normal_bias, 0.0f, 2.0f);
            ImGui::SliderFloat("Constant Bias", &shadow->constant_bias, 0.0f, 2.0f);
        }

        if (ImGui::CollapsingHeader("TAA"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto* taa = main_camera.renderer->get_feature<taa_feature>();

            static bool enable_taa = taa->is_enable();

            if (ImGui::Checkbox("Enable##TAA", &enable_taa))
            {
                if (enable_taa)
                {
                    taa->enable();
                }
                else
                {
                    taa->disable();
                }
            }
        }

        if (ImGui::CollapsingHeader("GTAO"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto* gtao = main_camera.renderer->get_feature<gtao_feature>();

            static bool enable_gtao = gtao->is_enable();
            if (ImGui::Checkbox("Enable##GTAO", &enable_gtao))
            {
                if (enable_gtao)
                {
                    gtao->enable();
                }
                else
                {
                    gtao->disable();
                }
            }

            if (enable_gtao)
            {
                ImGui::SliderInt("Slice Count", reinterpret_cast<int*>(&gtao->slice_count), 1, 5);
                ImGui::SliderInt("Step Count", reinterpret_cast<int*>(&gtao->step_count), 1, 5);
                ImGui::SliderFloat("Radius", &gtao->radius, 0.0f, 10.0f);
                ImGui::SliderFloat("Falloff", &gtao->falloff, 0.1f, 1.0f);
            }
        }

        if (ImGui::CollapsingHeader("Eye Adaptation"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto* eye_adaptation = main_camera.renderer->get_feature<eye_adaptation_feature>();

            static bool enable_eye_adaptation = eye_adaptation->is_enable();
            if (ImGui::Checkbox("Enable##EyeAdaptation", &enable_eye_adaptation))
            {
                if (enable_eye_adaptation)
                {
                    eye_adaptation->enable();
                }
                else
                {
                    eye_adaptation->disable();
                }
            }

            if (enable_eye_adaptation)
            {
                ImGui::SliderFloat("Min EV", &eye_adaptation->min_ev, -10.0f, 0.0f);
                ImGui::SliderFloat("Max EV", &eye_adaptation->max_ev, 0.0f, 10.0f);
                ImGui::SliderFloat("Low Percent", &eye_adaptation->low_percent, 0.0f, 1.0f);
                ImGui::SliderFloat("High Percent", &eye_adaptation->high_percent, 0.0f, 1.0f);
                ImGui::SliderFloat("Min Brightness", &eye_adaptation->min_brightness, 0.001f, 0.1f);
                ImGui::SliderFloat("Max Brightness", &eye_adaptation->max_brightness, 1.0f, 100.0f);
                ImGui::SliderFloat("Speed Down", &eye_adaptation->speed_down, 0.1f, 10.0f);
                ImGui::SliderFloat("Speed Up", &eye_adaptation->speed_up, 0.1f, 10.0f);
            }
        }

        if (ImGui::CollapsingHeader("Bloom"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto* bloom = main_camera.renderer->get_feature<bloom_feature>();

            static bool enable_bloom = bloom->is_enable();

            if (ImGui::Checkbox("Enable##Bloom", &enable_bloom))
            {
                if (enable_bloom)
                {
                    bloom->enable();
                }
                else
                {
                    bloom->disable();
                }
            }

            if (enable_bloom)
            {
                ImGui::SliderFloat("Threshold##Bloom", &bloom->threshold, 0.0f, 2.0f);
                ImGui::SliderFloat("Intensity##Bloom", &bloom->intensity, 0.0f, 2.0f);
                ImGui::SliderFloat("Knee##Bloom", &bloom->knee, 0.0f, 1.0f);
                ImGui::SliderFloat("Radius##Bloom", &bloom->radius, 0.2f, 1.0f);
            }
        }

        if (ImGui::CollapsingHeader("Profiling"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            imgui_profiling(main_camera.renderer->get_profiling());
        }

        if (ImGui::CollapsingHeader("Debug"))
        {
            auto& main_camera = world.get_component<camera_component>(get_camera());
            auto* renderer = static_cast<deferred_renderer*>(main_camera.renderer.get());

            static int debug_mode = static_cast<int>(renderer->get_debug_mode());
            const char* debug_mode_items[] = {
                "None",
                "Cluster",
                "Cluster Node",
                "Triangle",
                "VSM Page",
                "VSM Page Cache",
                "VSM Cull",
                "Shadow Mask",
                "Bloom",
                "Bloom Prefilter",
                "Eye Adaptation",
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
                auto* vsm = renderer->get_feature<vsm_feature>();
                vsm->set_debug_info(debug_info == 1);
            }

            if (debug_info == 1)
            {
                auto* vsm = renderer->get_feature<vsm_feature>();
                auto debug_info = vsm->get_debug_info();

                ImGui::Text("Cache Hit: %d", debug_info.cache_hit);
                ImGui::Text("Cache Miss: %d", debug_info.rendered);
                ImGui::Text("Unmapped: %d", debug_info.unmapped);
                ImGui::Text(
                    "Static Drawcall: %d",
                    debug_info.static_drawcall + debug_info.static_opacity_cutoff_drawcall);
                ImGui::Text(
                    "Dynamic Drawcall: %d",
                    debug_info.dynamic_drawcall + debug_info.dynamic_opacity_cutoff_drawcall);
            }
        }
    }

    entity m_root;

    std::unique_ptr<geometry> m_box_geometry;

    std::unique_ptr<pbr_material> m_pbr_material;
    std::unique_ptr<unlit_material> m_unlit_material;
};
} // namespace violet

int main()
{
    violet::application app("assets/config/hello-world.json");
    app.install<violet::hello_world>();
    app.run();

    return 0;
}