#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "control/control_module.hpp"
#include "core/engine.hpp"
#include "deferred_renderer.hpp"
#include "ecs/actor.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/graphics_module.hpp"
#include "graphics/materials/basic_material.hpp"
#include "scene/scene_module.hpp"
#include "window/window_module.hpp"
#include <filesystem>
#include <fstream>
#include <thread>

namespace violet::sample
{
class hello_world : public engine_module
{
public:
    hello_world() : engine_module("hello_world") {}

    virtual bool initialize(const dictionary& config)
    {
        get_module<window_module>().on_resize().then(
            [this](std::uint32_t width, std::uint32_t height)
            {
                resize(width, height);
            });

        initialize_render();
        initialize_scene();

        auto window_extent = get_module<window_module>().get_extent();
        resize(window_extent.width, window_extent.height);

        return true;
    }

    virtual void shutdown() {}

private:
    void initialize_render()
    {
        auto window_extent = get_module<window_module>().get_extent();

        rhi_swapchain_desc swapchain_desc = {};
        swapchain_desc.width = window_extent.width;
        swapchain_desc.height = window_extent.height;
        swapchain_desc.window_handle = get_module<window_module>().get_handle();
        m_swapchain = render_device::instance().create_swapchain(swapchain_desc);
        m_renderer = std::make_unique<deferred_renderer>();
    }

    void initialize_scene()
    {
        m_geometry = std::make_unique<box_geometry>();
        m_material = std::make_unique<basic_material>(float3{1.0f, 0.5f, 0.0f});

        m_cube = std::make_unique<actor>("cube", get_world());
        auto [cube_transform, cube_mesh] = m_cube->add<transform, mesh>();
        cube_mesh->set_geometry(m_geometry.get());
        cube_mesh->add_submesh(0, 0, 0, m_geometry->get_index_count(), m_material.get());

        m_main_camera = std::make_unique<actor>("camera", get_world());
        auto [camera_transform, main_camera, camera_control] =
            m_main_camera->add<transform, camera, orbit_control>();
        camera_transform->set_position(float3{0.0f, 0.0f, -10.0f});

        main_camera->set_renderer(m_renderer.get());
    }

    void resize(std::uint32_t width, std::uint32_t height)
    {
        render_device& device = render_device::instance();

        m_swapchain->resize(width, height);

        rhi_texture_desc depth_buffer_desc = {};
        depth_buffer_desc.extent.width = width;
        depth_buffer_desc.extent.height = height;
        depth_buffer_desc.format = RHI_FORMAT_D24_UNORM_S8_UINT;
        depth_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_buffer_desc.flags = RHI_TEXTURE_FLAG_DEPTH_STENCIL;
        m_depth_buffer = device.create_texture(depth_buffer_desc);

        rhi_texture_desc light_buffer_desc = {};
        light_buffer_desc.extent.width = width;
        light_buffer_desc.extent.height = height;
        light_buffer_desc.format = RHI_FORMAT_R8G8B8A8_UNORM;
        light_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        light_buffer_desc.flags = RHI_TEXTURE_FLAG_RENDER_TARGET | RHI_TEXTURE_FLAG_SHADER_RESOURCE;
        m_light_buffer = device.create_texture(light_buffer_desc);

        auto main_camera = m_main_camera->get<camera>();
        main_camera->resize(width, height);
        main_camera->set_render_target(0, m_swapchain.get());
        main_camera->set_render_target(1, m_depth_buffer.get());
        main_camera->set_render_target(2, m_light_buffer.get());
    }

    std::unique_ptr<deferred_renderer> m_renderer;
    rhi_ptr<rhi_swapchain> m_swapchain;
    rhi_ptr<rhi_texture> m_depth_buffer;
    rhi_ptr<rhi_texture> m_light_buffer;

    std::unique_ptr<material> m_material;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<actor> m_cube;
    std::unique_ptr<actor> m_main_camera;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine engine;
    engine.initialize("render-graph/config");
    engine.install<window_module>();
    engine.install<scene_module>();
    engine.install<graphics_module>();
    engine.install<control_module>();
    engine.install<sample::hello_world>();

    engine.get_module<window_module>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();

    return 0;
}