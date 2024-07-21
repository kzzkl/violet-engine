#include "graphics/passes/mesh_pass.hpp"

namespace violet
{
void mesh_pass::render(
    render_graph& graph,
    const render_context& context,
    const render_camera& camera,
    rdg_texture* render_target,
    rdg_texture* depth_buffer)
{
    struct pass_data : public rdg_data
    {
        render_list render_list;
        rdg_texture* render_target;
    };

    pass_data& data = graph.allocate_data<pass_data>();
    data.render_list = context.get_render_list(camera);
    data.render_target = render_target;

    auto pass = graph.add_pass<rdg_render_pass>("Mesh Pass");
    pass->add_render_target(render_target, RHI_ATTACHMENT_LOAD_OP_CLEAR);
    pass->set_depth_stencil(depth_buffer, RHI_ATTACHMENT_LOAD_OP_CLEAR);
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
            command->set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            command->draw_render_list(data.render_list);
        });
}
} // namespace violet