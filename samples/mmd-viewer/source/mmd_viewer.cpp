#include "mmd_viewer.hpp"
#include "components/camera_component.hpp"
#include "components/light_component.hpp"
#include "components/mmd_skeleton_component.hpp"
#include "components/morph_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "core/engine.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "imgui.h"
#include "task/task_graph_printer.hpp"
#include "window/window_system.hpp"

namespace violet
{
mmd_viewer::mmd_viewer()
    : engine_system("mmd viewer")
{
}

mmd_viewer::~mmd_viewer() {}

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
        []()
        {
            engine::exit();
        });

    task_graph& task_graph = get_task_graph();
    task_group& update = task_graph.get_group("Update");

    task_graph.add_task()
        .set_name("MMD Tick")
        .set_group(update)
        .set_options(TASK_OPTION_MAIN_THREAD)
        .set_execute(
            [this]()
            {
                tick();
            });

    task_graph.reset();
    task_graph_printer::print(task_graph);

    initialize_render();
    initialize_scene();

    resize();

    return true;
}

void mmd_viewer::initialize_render()
{
    auto window_extent = get_system<window_system>().get_extent();

    m_swapchain = render_device::instance().create_swapchain({
        .extent =
            {
                .width = window_extent.width,
                .height = window_extent.height,
            },
        .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
        .window_handle = get_system<window_system>().get_handle(),
    });
    m_renderer = std::make_unique<mmd_renderer>();

    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon01.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon02.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon03.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon04.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon05.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon06.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon07.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon08.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon09.bmp"));
    m_internal_toons.push_back(texture_loader::load("assets/mmd/toon10.bmp"));
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
    camera.render_targets = {m_swapchain.get()};

    m_light = world.create();
    world.add_component<light_component, transform_component, scene_component>(m_light);

    auto& light_transform = world.get_component<transform_component>(m_light);
    light_transform.lookat({-1.0f, -1.0f, 1.0f});

    auto& light = world.get_component<light_component>(m_light);
    light.type = LIGHT_DIRECTIONAL;
    light.color = {1.0f, 1.0f, 1.0f};

    std::vector<rhi_texture*> internal_toons(m_internal_toons.size());
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
        dirty = ImGui::SliderFloat("Rotate X", &rotation.x, 0.0f, math::PI) || dirty;
        dirty = ImGui::SliderFloat("Rotate Y", &rotation.y, 0.0f, math::TWO_PI) || dirty;

        if (dirty)
        {
            auto& transform = world.get_component<transform_component>(m_light);

            vec4f_simd q = quaternion::from_euler(math::load(rotation));
            transform.set_rotation(q);
        }

        static vec3f color = {};
        if (ImGui::ColorEdit3("Color", &color.r))
        {
            auto& light = world.get_component<light_component>(m_light);
            light.color = color;
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
}

void mmd_viewer::resize()
{
    auto extent = get_system<window_system>().get_extent();

    m_swapchain->resize(extent.width, extent.height);

    auto& main_camera = get_world().get_component<camera_component>(m_camera);
    main_camera.render_targets[0] = m_swapchain.get();
}
} // namespace violet