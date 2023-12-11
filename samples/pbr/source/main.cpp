#include "components/camera.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_system.hpp"
#include "pbr_render.hpp"
#include "scene/scene_system.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class pbr_sample : public engine_system
{
public:
    pbr_sample() : engine_system("pbr sample") {}

    bool initialize(const dictionary& config) override
    {
        on_tick().then(
            [this](float delta)
            {
                tick(delta);
            });

        auto& window = get_system<window_system>();
        window.on_resize().then(
            [this](std::uint32_t width, std::uint32_t height)
            {
                resize(width, height);
            });

        renderer* renderer = get_system<graphics_system>().get_renderer();
        m_render_graph = std::make_unique<pbr_render_graph>(renderer);

        m_camera = std::make_unique<actor>("main camera", get_world());
        auto [camera_transform, main_camera, camera_control] =
            m_camera->add<transform, camera, orbit_control>();
        main_camera->set_render_pass(m_render_graph->get_render_pass("main"));

        auto extent = window.get_extent();
        resize(extent.width, extent.height);

        m_skybox = renderer->create_texture_cube(
            "pbr/skybox/icebergs/right.jpg",
            "pbr/skybox/icebergs/left.jpg",
            "pbr/skybox/icebergs/top.jpg",
            "pbr/skybox/icebergs/bottom.jpg",
            "pbr/skybox/icebergs/front.jpg",
            "pbr/skybox/icebergs/back.jpg");

        rhi_sampler_desc sampler_desc = {};
        sampler_desc.min_filter = RHI_FILTER_LINEAR;
        sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        m_skybox_sampler = renderer->create_sampler(sampler_desc);

        main_camera->set_skybox(m_skybox.get(), m_skybox_sampler.get());

        return true;
    }

private:
    void tick(float delta) { get_system<graphics_system>().render(m_render_graph.get()); }

    void resize(std::uint32_t width, std::uint32_t height)
    {
        rhi_depth_stencil_buffer_desc depth_stencil_buffer_desc = {};
        depth_stencil_buffer_desc.width = width;
        depth_stencil_buffer_desc.height = height;
        depth_stencil_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_stencil_buffer_desc.format = RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT;
        m_depth_stencil = get_system<graphics_system>().get_renderer()->create_depth_stencil_buffer(
            depth_stencil_buffer_desc);

        auto main_camera = m_camera->get<camera>();
        main_camera->resize(width, height);
        main_camera->set_attachment(0, nullptr, true);
        main_camera->set_attachment(1, m_depth_stencil.get());
    }

    std::unique_ptr<pbr_render_graph> m_render_graph;
    rhi_ptr<rhi_resource> m_depth_stencil;

    rhi_ptr<rhi_resource> m_skybox;
    rhi_ptr<rhi_sampler> m_skybox_sampler;

    std::unique_ptr<actor> m_camera;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine engine;
    engine.initialize("");
    engine.install<scene_system>();
    engine.install<window_system>();
    engine.install<graphics_system>();
    engine.install<control_system>();
    engine.install<sample::pbr_sample>();

    engine.get_system<window_system>().on_destroy().then(
        [&engine]()
        {
            engine.exit();
        });

    engine.run();

    return 0;
}