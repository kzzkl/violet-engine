#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/transform.hpp"
#include "components/ui_root.hpp"
#include "core/engine.hpp"
#include "ecs/actor.hpp"
#include "gallery.hpp"
#include "graphics/graphics_module.hpp"
#include "graphics/passes/present_pass.hpp"
#include "scene/scene_module.hpp"
#include "ui/ui_module.hpp"
#include "ui/widgets/button.hpp"
#include "ui/widgets/label.hpp"
#include "window/window_module.hpp"

namespace violet::sample
{
class sample_module : public engine_module
{
public:
    sample_module() : engine_module("sample") {}

    virtual bool initialize(const dictionary& config) override
    {
        auto& window = get_module<window_module>();
        window.on_resize().then(
            [this](std::uint32_t width, std::uint32_t height)
            {
                resize(width, height);
            });

        on_tick().then(
            [this](float delta)
            {
            });

        initialize_ui();

        return true;
    }

private:
    void initialize_ui()
    {
        auto& window = get_module<window_module>();
        auto& graphics = get_module<graphics_module>();

        render_device* device = graphics.get_device();

        auto window_extent = window.get_extent();
        m_swapchain = device->create_swapchain(
            rhi_swapchain_desc{window_extent.width, window_extent.height, window.get_handle()});

        m_render_graph = std::make_unique<render_graph>();

        rdg_texture* render_target =
            m_render_graph->add_resource<rdg_texture>("render target", true);
        render_target->set_format(m_swapchain->get_texture()->get_format());

        ui_pass* ui = m_render_graph->add_pass<ui_pass>("ui pass");
        present_pass* present = m_render_graph->add_pass<present_pass>("present pass");

        m_render_graph->add_edge(render_target, ui, "render target", RDG_EDGE_OPERATE_CLEAR);
        m_render_graph->add_edge(ui, "render target", present, "target", RDG_EDGE_OPERATE_STORE);

        m_render_graph->compile(device);

        m_main_camera = std::make_unique<actor>("main camera", get_world());
        auto [main_camera, main_ui, camera_transform] =
            m_main_camera->add<camera, ui_root, transform>();

        main_camera->set_render_graph(m_render_graph.get());
        main_camera->set_render_texture("render target", m_swapchain.get());

        main_ui->set_pass(ui);
        main_ui->get_container()->add<gallery>();

        resize(window_extent.width, window_extent.height);
    }

    void resize(std::uint32_t width, std::uint32_t height)
    {
        m_main_camera->get<camera>()->resize(width, height);
        m_swapchain->resize(width, height);
    }

    rhi_ptr<rhi_swapchain> m_swapchain;
    rhi_ptr<rhi_texture> m_depth;
    std::unique_ptr<render_graph> m_render_graph;

    std::unique_ptr<actor> m_main_camera;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine engine;
    engine.initialize("hello-world/config");
    engine.install<window_module>();
    engine.install<scene_module>();
    engine.install<graphics_module>();
    engine.install<ui_module>();
    engine.install<sample::sample_module>();

    engine.get_module<window_module>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();
}