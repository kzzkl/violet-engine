#include "graphics/passes/mesh_pass.hpp"

namespace violet
{
void mesh_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        const render_scene& scene;
        const render_camera& camera;

        rdg_buffer* command_buffer;
        rdg_buffer* count_buffer;

        material_type material_type;
    };

    pass_data data = {
        .scene = parameter.scene,
        .camera = parameter.camera,
        .command_buffer = parameter.command_buffer,
        .count_buffer = parameter.count_buffer,
        .material_type = parameter.material_type,
    };

    auto& pass = graph.add_pass<rdg_render_pass>("Mesh Pass");
    pass.add_buffer(
        data.command_buffer,
        RHI_PIPELINE_STAGE_DRAW_INDIRECT,
        RHI_ACCESS_INDIRECT_COMMAND_READ);
    pass.add_buffer(
        data.count_buffer,
        RHI_PIPELINE_STAGE_DRAW_INDIRECT,
        RHI_ACCESS_INDIRECT_COMMAND_READ);

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
    pass.set_execute(
        [data](rdg_command& command)
        {
            command.set_viewport(data.camera.viewport);
            command.set_scissor(data.camera.scissor_rects);

            command.draw_instances(
                data.scene,
                data.camera,
                data.command_buffer->get_rhi(),
                data.count_buffer->get_rhi(),
                data.material_type);
        });
}
} // namespace violet