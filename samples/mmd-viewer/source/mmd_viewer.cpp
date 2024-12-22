#include "mmd_viewer.hpp"
#include "components/camera_component.hpp"
#include "components/orbit_control_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/renderers/deferred_renderer.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "window/window_system.hpp"

namespace violet::sample
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

    auto& window = get_system<window_system>();
    window.on_resize().add_task().set_execute(
        [this]()
        {
            resize();
        });
    window.on_destroy().add_task().set_execute(
        []()
        {
            // engine::exit();
        });

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
    m_renderer = std::make_unique<deferred_renderer>();

    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon01.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon02.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon03.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon04.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon05.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon06.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon07.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon08.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon09.bmp"));
    m_internal_toons.push_back(texture_loader::load("mmd-viewer/mmd/toon10.bmp"));
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

    auto& main_camera = world.get_component<camera_component>(m_camera);
    main_camera.renderer = m_renderer.get();
    main_camera.render_targets = {m_swapchain.get()};

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

void mmd_viewer::tick(float delta) {}

void mmd_viewer::resize()
{
    auto extent = get_system<window_system>().get_extent();

    m_swapchain->resize(extent.width, extent.height);

    auto& main_camera = get_world().get_component<camera_component>(m_camera);
    main_camera.render_targets[0] = m_swapchain.get();
}
} // namespace violet::sample