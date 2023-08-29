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
        window.on_resize().then([this](std::uint32_t width, std::uint32_t height)
                                { log::info("Window resize: {} {}", width, height); });

        engine::on_tick().then(
            [this](float delta)
            {
                tick(delta);
                engine::get_system<graphics_system>().render(m_render_graph.get());
            });

        m_test_object = std::make_unique<node>("test", engine::get_world());

        initialize_render();

        return true;
    }

    virtual void shutdown()
    {
        m_render_graph = nullptr;
        rhi_context* rhi = engine::get_system<graphics_system>().get_rhi();
        rhi->destroy_vertex_buffer(m_position_buffer);
        rhi->destroy_vertex_buffer(m_color_buffer);
        rhi->destroy_index_buffer(m_index_buffer);

        rhi->destroy_pipeline_parameter(m_mvp);
        rhi->destroy_pipeline_parameter_layout(m_mvp_layout);
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
        output.set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
        output.set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

        render_subpass& color_pass = main.add_subpass("color");
        color_pass.add_reference(
            output,
            RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
            RHI_RESOURCE_STATE_RENDER_TARGET);

        render_pipeline& pipeline = color_pass.add_pipeline("color");
        pipeline.set_shader(
            "hello-world/shaders/base.vert.spv",
            "hello-world/shaders/base.frag.spv");
        pipeline.set_vertex_layout({
            {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
            {"color",    RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
        });
        pipeline.set_cull_mode(RHI_CULL_MODE_NONE);

        rhi_pipeline_parameter_layout_desc layout_desc = {};
        layout_desc.parameters[0] = {RHI_PIPELINE_PARAMETER_TYPE_UNIFORM_BUFFER, sizeof(float4x4)};
        layout_desc.parameter_count = 1;
        m_mvp_layout = graphics.get_rhi()->create_pipeline_parameter_layout(layout_desc);
        pipeline.set_parameter_layout({m_mvp_layout});

        m_render_graph->compile();

        std::vector<float3> position = {
            float3{-0.5f, -0.5f, 0.0f},
            float3{0.5f,  -0.5f, 0.0f},
            float3{0.5f,  0.5f,  0.0f},
            float3{-0.5f, 0.5f,  0.0f}
        };
        rhi_vertex_buffer_desc position_buffer_desc = {};
        position_buffer_desc.data = position.data();
        position_buffer_desc.size = position.size() * sizeof(float3);
        position_buffer_desc.dynamic = false;
        m_position_buffer = graphics.get_rhi()->create_vertex_buffer(position_buffer_desc);

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
        m_color_buffer = graphics.get_rhi()->create_vertex_buffer(color_buffer_desc);

        std::vector<std::uint32_t> indices = {0, 1, 2, 2, 3, 0};
        rhi_index_buffer_desc index_buffer_desc = {};
        index_buffer_desc.data = indices.data();
        index_buffer_desc.size = indices.size() * sizeof(std::uint32_t);
        index_buffer_desc.index_size = sizeof(std::uint32_t);
        index_buffer_desc.dynamic = false;
        m_index_buffer = graphics.get_rhi()->create_index_buffer(index_buffer_desc);

        m_mvp = graphics.get_rhi()->create_pipeline_parameter(m_mvp_layout);

        m_pipeline = &pipeline;

        m_texture = graphics.get_rhi()->create_texture("hello-world/test.jpg");
    }

    render_pipeline* m_pipeline;

    void tick(float delta)
    {
        auto& window = engine::get_system<window_system>();
        auto rect = window.get_extent();

        if (rect.width == 0 || rect.height == 0)
            return;

        float4x4_simd p = matrix_simd::perspective(
            to_radians(45.0f),
            static_cast<float>(rect.width) / static_cast<float>(rect.height),
            0.1f,
            100.0f);

        float4x4_simd m = matrix_simd::affine_transform(
            simd::set(10.0, 10.0, 10.0, 0.0),
            quaternion_simd::rotation_axis(simd::set(1.0f, 0.0f, 0.0f, 0.0f), m_rotate),
            simd::set(0.0, 0.0, 0.0, 0.0));

        float4x4_simd v = matrix_simd::affine_transform(
            simd::set(1.0f, 1.0f, 1.0f, 0.0f),
            simd::set(0.0f, 0.0f, 0.0f, 1.0f),
            simd::set(0.0, 0.0, -30.0f, 0.0f));
        v = matrix_simd::inverse_transform(v);

        float4x4_simd mvp = matrix_simd::mul(matrix_simd::mul(m, v), p);
        float4x4 data;
        simd::store(mvp, data);

        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time =
            std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
                .count();

        std::size_t index =
            engine::get_system<graphics_system>().get_rhi()->get_frame_resource_index();
        m_mvp->set(0, &mvp, sizeof(mvp), 0);

        m_pipeline->set_mesh({m_position_buffer, m_color_buffer}, m_index_buffer, m_mvp);

        m_rotate += delta * 1.0f;
    }

    std::unique_ptr<node> m_test_object;

    std::unique_ptr<render_graph> m_render_graph;

    rhi_resource* m_position_buffer;
    rhi_resource* m_color_buffer;
    rhi_resource* m_index_buffer;

    rhi_pipeline_parameter_layout* m_mvp_layout;
    rhi_pipeline_parameter* m_mvp;

    rhi_resource* m_texture;

    float m_rotate = 0.0f;
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