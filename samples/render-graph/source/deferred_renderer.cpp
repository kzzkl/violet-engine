#include "deferred_renderer.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"

namespace violet::sample
{
deferred_renderer::deferred_renderer()
{
}

void deferred_renderer::render(
    render_graph& graph,
    const render_context& context,
    const render_camera& camera)
{
    m_render_target = graph.add_resource<rdg_texture>(
        "Render Target",
        camera.render_targets[0],
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_PRESENT);
    m_depth_buffer = graph.add_resource<rdg_texture>(
        "Depth Buffer",
        camera.render_targets[1],
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

    add_mesh_pass(graph, context, camera);
}

void deferred_renderer::add_mesh_pass(
    render_graph& graph,
    const render_context& context,
    const render_camera& camera)
{
    class pass_data : public rdg_data
    {
    public:
        virtual void reset() override {}

        render_list render_list;

        rdg_texture* render_target;
    };

    pass_data& data = graph.allocate_data<pass_data>();
    data.render_list = context.get_render_list(camera);
    data.render_target = m_render_target;

    auto pass = graph.add_pass<rdg_render_pass>("Mesh Pass");
    pass->add_render_target(m_render_target, RHI_ATTACHMENT_LOAD_OP_CLEAR);
    pass->set_depth_stencil(m_depth_buffer, RHI_ATTACHMENT_LOAD_OP_CLEAR);
    pass->set_execute(
        [&data](rdg_command* command)
        {
            rhi_texture_extent extent = data.render_target->get_rhi()->get_extent();
            rhi_viewport viewport = {
                .x = 0,
                .y = 0,
                .width = static_cast<float>(extent.width),
                .height = static_cast<float>(extent.height),
                .min_depth = 0.0f,
                .max_depth = 1.0f};
            command->set_viewport(viewport);

            rhi_scissor_rect scissor =
                {.min_x = 0, .min_y = 0, .max_x = extent.width, .max_y = extent.height};
            command->set_scissor({scissor});

            command->draw_render_list(data.render_list);
        });
}
} // namespace violet::sample