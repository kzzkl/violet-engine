#include "graphics/renderers/passes/mesh_pass.hpp"

namespace violet
{
void mesh_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_buffer_ref command_buffer;
        rdg_buffer_ref count_buffer;

        material_type material_type;
    };

    graph.add_pass<pass_data>(
        "Mesh Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            rhi_attachment_load_op load_op =
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;
            for (auto* render_target : parameter.render_targets)
            {
                pass.add_render_target(render_target, load_op);
            }
            if (parameter.depth_buffer != nullptr)
            {
                pass.set_depth_stencil(parameter.depth_buffer, load_op);
            }

            data.command_buffer = pass.add_buffer(
                parameter.command_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.count_buffer = pass.add_buffer(
                parameter.count_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.material_type = parameter.material_type;
        },
        [&](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            command.draw_instances(
                data.command_buffer.get_rhi(),
                data.count_buffer.get_rhi(),
                data.material_type);
        });
}
} // namespace violet