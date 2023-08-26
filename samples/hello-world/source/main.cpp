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
        window.on_resize().then([](std::uint32_t width, std::uint32_t height)
                                { log::info("Window resize: {} {}", width, height); });
        window.on_keyboard_key().then([](keyboard_key key, key_state state)
                                      { log::info("{} {}", key, state.down()); });

        engine::on_tick().then(
            [this](float delta)
            { engine::get_system<graphics_system>().render(m_render_graph.get()); });

        m_test_object = std::make_unique<node>("test", engine::get_world());

        initialize_render();

        return true;
    }

    virtual void shutdown()
    {
        m_render_graph = nullptr;
        auto& graphics = engine::get_system<graphics_system>();
        graphics.get_rhi()->destroy_vertex_buffer(m_position_buffer);
        graphics.get_rhi()->destroy_vertex_buffer(m_color_buffer);
        graphics.get_rhi()->destroy_index_buffer(m_index_buffer);
    }

private:
    void initialize_render()
    {
        auto& graphics = engine::get_system<graphics_system>();
        m_render_graph = std::make_unique<render_graph>(graphics.get_rhi());

        render_resource& back_buffer = m_render_graph->add_resource("back buffer", true);

        render_pass& main = m_render_graph->add_render_pass("main");

        render_attachment& output = main.add_attachment("output", back_buffer);
        output.set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
        output.set_final_state(RHI_RESOURCE_STATE_PRESENT);
        output.set_load_op(RHI_ATTACHMENT_LOAD_OP_DONT_CARE);

        render_subpass& color_pass = main.add_subpass("color");
        color_pass.add_reference(
            output,
            RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
            RHI_RESOURCE_STATE_RENDER_TARGET);

        render_pipeline& pipeline = color_pass.add_pipeline("color");
        pipeline.set_shader(
            "hello-world/resource/shaders/base.vert.spv",
            "hello-world/resource/shaders/base.frag.spv");
        pipeline.set_vertex_layout({
            {"position", RHI_RESOURCE_FORMAT_R32G32_FLOAT   },
            {"color",    RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
        });

        m_render_graph->compile();

        std::vector<float2> position = {
            float2{-0.5f, 0.5f },
            float2{0.5f,  0.5f },
            float2{-0.5f, -0.5f},
            float2{0.5f,  -0.5f}
        };
        rhi_vertex_buffer_desc position_buffer_desc = {};
        position_buffer_desc.data = position.data();
        position_buffer_desc.size = position.size() * sizeof(float2);
        position_buffer_desc.dynamic = false;
        m_position_buffer = graphics.get_rhi()->make_vertex_buffer(position_buffer_desc);

        std::vector<float3> color = {
            float3{1.0f, 0.0f, 0.0f},
            float3{0.0f, 1.0f, 0.0f},
            float3{0.0f, 1.0f, 1.0f},
            float3{0.0f, 0.0f, 1.0f},
        };
        rhi_vertex_buffer_desc color_buffer_desc = {};
        color_buffer_desc.data = color.data();
        color_buffer_desc.size = color.size() * sizeof(float3);
        color_buffer_desc.dynamic = false;
        m_color_buffer = graphics.get_rhi()->make_vertex_buffer(color_buffer_desc);

        std::vector<std::uint32_t> indices = {0, 2, 1, 1, 2, 3};
        rhi_index_buffer_desc index_buffer_desc = {};
        index_buffer_desc.data = indices.data();
        index_buffer_desc.size = indices.size() * sizeof(std::uint32_t);
        index_buffer_desc.index_size = sizeof(std::uint32_t);
        index_buffer_desc.dynamic = false;
        m_index_buffer = graphics.get_rhi()->make_index_buffer(index_buffer_desc);

        pipeline.set_mesh({m_position_buffer, m_color_buffer}, m_index_buffer);
    }

    std::unique_ptr<node> m_test_object;

    std::unique_ptr<render_graph> m_render_graph;

    rhi_resource* m_position_buffer;
    rhi_resource* m_color_buffer;
    rhi_resource* m_index_buffer;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine::initialize("");
    engine::install<window_system>();
    engine::install<graphics_system>();
    engine::install<sample::hello_world>();

    engine::get_system<window_system>().on_destroy().then(
        []()
        {
            log::info("Close window");
            engine::exit();
        });

    engine::run();

    return 0;
}