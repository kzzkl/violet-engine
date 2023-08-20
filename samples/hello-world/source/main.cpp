#include "core/engine.hpp"
#include "core/node/node.hpp"
#include "graphics/graphics_system.hpp"
#include "window/window_system.hpp"
#include <filesystem>
#include <fstream>
#include <thread>

namespace violet::sample
{
class hello_world : public engine_system
{
public:
    hello_world() : engine_system("hello_world") {}

    virtual bool initialize(const dictionary& config)
    {
        log::info(config["text"]);

        auto& window = engine::get_system<window_system>();
        window.on_destroy().then(
            []()
            {
                log::info("Close window");
                engine::exit();
            });
        window.on_resize().then([](std::uint32_t width, std::uint32_t height)
                                { log::info("Window resize: {} {}", width, height); });
        window.on_keyboard_key().then([](keyboard_key key, key_state state)
                                      { log::info("{} {}", key, state.down()); });

        engine::on_tick().then(
            [this](float delta)
            { engine::get_system<graphics_system>().render(m_render_graph.get()); });

        m_test_object = std::make_unique<node>("test");

        initialize_render();

        return true;
    }

    virtual void shutdown() {}

private:
    void initialize_render()
    {
        auto& graphics = engine::get_system<graphics_system>();
        m_render_graph = std::make_unique<render_graph>(graphics.get_rhi());

        render_resource* back_buffer = m_render_graph->get_back_buffer();

        render_pass* main = m_render_graph->add_render_pass("main");

        render_attachment* output = main->add_attachment("output", back_buffer);
        output->set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
        output->set_final_state(RHI_RESOURCE_STATE_PRESENT);
        output->set_load_op(RHI_ATTACHMENT_LOAD_OP_DONT_CARE);

        render_subpass* color_pass = main->add_subpass("color");
        color_pass->add_reference(
            output,
            RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
            RHI_RESOURCE_STATE_RENDER_TARGET);

        render_pipeline* pipeline = color_pass->add_pipeline("color");
        pipeline->set_shader(
            "hello-world/resource/shaders/base.vert.spv",
            "hello-world/resource/shaders/base.frag.spv");

        m_render_graph->compile();
    }

    std::unique_ptr<node> m_test_object;

    std::unique_ptr<render_graph> m_render_graph;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine::initialize("");
    engine::install<window_system>();
    engine::install<graphics_system>();
    engine::install<sample::hello_world>();
    engine::run();

    return 0;
}