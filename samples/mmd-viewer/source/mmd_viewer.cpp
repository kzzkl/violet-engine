#include "mmd_viewer.hpp"
#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mmd_skeleton_component.hpp"
#include "components/morph_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/passes/taa_pass.hpp"
#include "imgui.h"
#include "imgui_system.hpp"
#include "mmd_animation.hpp"
#include "mmd_material.hpp"
#include "physics/physics_system.hpp"
#include "window/window_system.hpp"

namespace violet
{
mmd_viewer::mmd_viewer()
    : system("mmd viewer")
{
}

mmd_viewer::~mmd_viewer() {}

void mmd_viewer::install(application& app)
{
    app.install<window_system>();
    app.install<graphics_system>();
    app.install<physics_system>();
    app.install<control_system>();
    app.install<imgui_system>();
    app.install<mmd_animation>();

    m_app = &app;
}

bool mmd_viewer::initialize(const dictionary& config)
{
    m_pmx_path = config["pmx"];
    m_vmd_path = config["vmd"];

    auto& world = get_world();
    world.register_component<mmd_skeleton_component>();

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

    auto& task_graph = get_task_graph();
    auto& update = task_graph.get_group("Update");

    task_graph.add_task()
        .set_name("MMD Tick")
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

void mmd_viewer::initialize_render()
{
    m_swapchain = render_device::instance().create_swapchain({
        .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
        .window_handle = get_system<window_system>().get_handle(),
    });
    m_renderer = std::make_unique<mmd_renderer>();

    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon01.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon02.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon03.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon04.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon05.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon06.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon07.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon08.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon09.bmp"));
    m_internal_toons.push_back(std::make_unique<texture_2d>("assets/mmd/toon10.bmp"));
}

void mmd_viewer::initialize_scene()
{
    auto& world = get_world();

    m_camera = world.create();
    world.add_component<
        transform_component,
        camera_component,
        orbit_control_component,
        scene_component>(m_camera);

    auto& camera_transform = world.get_component<transform_component>(m_camera);
    camera_transform.set_position({0.0f, 0.0f, -40.0f});

    auto& camera_control = world.get_component<orbit_control_component>(m_camera);
    camera_control.target = {0.0f, 10.0f, 0.0f};
    camera_control.r = 40.0f;

    auto& camera = world.get_component<camera_component>(m_camera);
    camera.renderer = m_renderer.get();
    camera.render_target = m_swapchain.get();
    camera.features.push_back(std::make_unique<taa_render_feature>());

    m_light = world.create();
    world.add_component<light_component, transform_component, scene_component>(m_light);

    auto& light_transform = world.get_component<transform_component>(m_light);
    light_transform.lookat({-1.0f, -1.0f, 1.0f});

    auto& light = world.get_component<light_component>(m_light);
    light.type = LIGHT_DIRECTIONAL;
    light.color = {10.0f, 10.0f, 10.0f};

    std::vector<texture_2d*> internal_toons(m_internal_toons.size());
    std::transform(
        m_internal_toons.begin(),
        m_internal_toons.end(),
        internal_toons.begin(),
        [](auto& texture)
        {
            return texture.get();
        });

    mmd_loader loader(internal_toons);
    if (auto result = loader.load(m_pmx_path, m_vmd_path, get_world()))
    {
        m_model_data = std::move(*result);
    }
}

void mmd_viewer::tick()
{
    auto& world = get_world();

    if (ImGui::CollapsingHeader("Light"))
    {
        bool dirty = false;

        static vec3f rotation;
        dirty = ImGui::SliderFloat("Rotate X", &rotation.x, -math::PI, math::PI) || dirty;
        dirty = ImGui::SliderFloat("Rotate Y", &rotation.y, 0.0f, math::TWO_PI) || dirty;

        if (dirty)
        {
            auto& transform = world.get_component<transform_component>(m_light);

            vec4f_simd q = quaternion::from_euler(math::load(rotation));
            transform.set_rotation(q);
        }

        static vec3f color = {1.0f, 1.0f, 1.0f};
        static float strength = 10.0f;

        if (ImGui::ColorEdit3("Color", &color.r))
        {
            auto& light = world.get_component<light_component>(m_light);
            light.color = color * strength;
        }

        if (ImGui::SliderFloat("Strength", &strength, 0.0f, 500))
        {
            auto& light = world.get_component<light_component>(m_light);
            light.color = color * strength;
        }
    }

    if (ImGui::CollapsingHeader("Material"))
    {
        static float outline_width = 1.0f;
        static float outline_z_offset = 0.0f;

        bool dirty = false;
        if (ImGui::SliderFloat("Outline Width", &outline_width, 0.0f, 1.0f))
        {
            dirty = true;
        }

        if (ImGui::SliderFloat("Outline Z Offset", &outline_z_offset, 0.0f, 1.0f))
        {
            dirty = true;
        }

        if (dirty)
        {
            for (std::size_t i = 0; i < m_model_data.materials.size(); ++i)
            {
                auto* outline_material =
                    static_cast<mmd_outline_material*>(m_model_data.outline_materials[i].get());

                if (outline_material != nullptr)
                {
                    outline_material->set_width(outline_width);
                    outline_material->set_z_offset(outline_z_offset);
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Morph"))
    {
        auto& morph = world.get_component<morph_component>(m_model_data.root);
        for (std::size_t i = 0; i < morph.weights.size(); ++i)
        {
            std::string t = std::to_string(i);
            ImGui::SliderFloat(t.c_str(), &morph.weights[i], 0.0f, 1.0f);
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
}

void mmd_viewer::resize()
{
    m_swapchain->resize();
}
} // namespace violet