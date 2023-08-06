#include "components/mesh.hpp"
#include "core/context/engine.hpp"
#include "core/node/node.hpp"
#include "graphics/graphics_module.hpp"
#include "graphics/node_parameter.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "window/window_module.hpp"
#include "window/window_task.hpp"
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
        auto& window = engine::get_module<window_module>();
        window.get_task_graph().window_destroy.add_task(
            "exit",
            []()
            {
                log::info("close window");
                engine::exit();
            });

        initialize_render_graph();

        return true;
    }

    virtual void shutdown() {}

private:
    void initialize_render_graph()
    {
        rhi_context* rhi = engine::get_module<graphics_module>().get_rhi();
        render_graph* render_graph = engine::get_module<graphics_module>().get_render_graph();

        render_pass* pass = render_graph->add_render_pass("test pass");

        pass->set_attachment_count(1);
        rhi_attachment_desc color = {};
        color.format = rhi->get_back_buffer()->get_format();
        color.samples = 1;
        color.initial_state = RHI_RESOURCE_STATE_UNDEFINED;
        color.final_state = RHI_RESOURCE_STATE_PRESENT;
        color.load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        pass->set_attachment_description(0, color);

        pass->set_subpass_count(1);

        std::vector<rhi_attachment_reference> references = {
            {RHI_ATTACHMENT_REFERENCE_TYPE_COLOR, RHI_RESOURCE_STATE_RENDER_TARGET, 0}
        };
        pass->set_subpass_references(0, references);
        pass->compile();

        m_pipeline = pass->add_pipeline("test pipeline", 0);
        m_pipeline->set_shader(
            "render-graph/resource/shaders/base.vert.spv",
            "render-graph/resource/shaders/base.frag.spv");
        // pipeline->set_vertex_layout({
        //     {"position", RESOURCE_FORMAT_R32G32B32_FLOAT},
        //     {"color",    RESOURCE_FORMAT_R32G32B32_FLOAT}
        // });
        // pipeline_parameter_desc camera = {
        //     .parameters = {{PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, 128}},
        //     .parameter_count = 1};
        // pipeline_parameter_desc object = {
        //     .parameters = {{PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, 128}},
        //     .parameter_count = 1};
        // pipeline->set_parameter_layout({camera, object});
        m_pipeline->compile();

        rhi_framebuffer* framebuffer = rhi->make_framebuffer(
            rhi_framebuffer_desc{pass->get_interface(), {rhi->get_back_buffer()}, 1});

        rhi_render_command* command = rhi->allocate_command();
        command->begin(pass->get_interface(), framebuffer);

        rhi_resource_extent extent = rhi->get_back_buffer()->get_extent();
        command->set_viewport(0, 0, extent.width, extent.height, 0.0, 1.0);

        std::vector<rhi_scissor_rect> scissors = {
            {0, 0, extent.width, extent.height}
        };
        command->set_scissor(scissors.data(), scissors.size());

        command->set_pipeline(m_pipeline->get_interface());

        command->draw(0, 3);
        command->end();

        rhi->begin_frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        rhi->execute(command, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        rhi->present();
    }

    void initialize_object()
    {
        m_object = std::make_unique<node>("test");
        m_object->add_component<mesh>();
        auto m = m_object->get_component<mesh>();

        m_material = std::make_unique<material>();
        m_material->add_pipeline(m_pipeline);

        m->set_material_count(1);
        m->set_material(0, m_material.get());

        m->set_submesh_count(1);
        m->set_submesh(
            0,
            {.index_begin = 0, .index_end = 3, .vertex_offset = 0, .material_index = 0});
    }

    render_pipeline* m_pipeline;

    std::unique_ptr<node> m_object;
    std::unique_ptr<material> m_material;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine::initialize("");
    engine::install<window_module>();
    engine::install<graphics_module>();
    engine::install<sample::hello_world>();
    engine::run();

    return 0;
}