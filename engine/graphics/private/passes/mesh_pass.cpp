#include "graphics/passes/mesh_pass.hpp"

namespace violet
{
void mesh_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data : public rdg_data
    {
        rhi_parameter* camera;
        rhi_viewport viewport;

        render_list render_list;
    };

    pass_data& data = graph.allocate_data<pass_data>();
    data.viewport = parameter.viewport;
    data.render_list = parameter.render_list;

    rdg_render_pass* pass = graph.add_pass<rdg_render_pass>("Mesh Pass");

    pass->add_render_target(
        parameter.render_target,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass->set_depth_stencil(
        parameter.depth_buffer,
        parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
    pass->set_execute(
        [&data](rdg_command& command)
        {
            command.set_viewport(data.viewport);

            rhi_scissor_rect scissor = {
                .min_x = 0,
                .min_y = 0,
                .max_x = static_cast<std::uint32_t>(data.viewport.width),
                .max_y = static_cast<std::uint32_t>(data.viewport.height)};
            command.set_scissor(std::span<rhi_scissor_rect>(&scissor, 1));

            command.draw_render_list(data.render_list);
        });
}
} // namespace violet