#include "graphics/passes/depth_only_pass.hpp"

namespace violet
{
struct depth_only_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/depth_only.hlsl";
};

void depth_only_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_buffer_ref command_buffer;
        rdg_buffer_ref count_buffer;

        material_type material_type;

        rdg_raster_pipeline pipeline;
    };

    graph.add_pass<pass_data>(
        "Depth Only Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.set_depth_stencil(
                parameter.depth_buffer,
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);

            data.command_buffer = pass.add_buffer(
                parameter.command_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.count_buffer = pass.add_buffer(
                parameter.count_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.material_type = parameter.material_type;

            auto& device = render_device::instance();

            data.pipeline = {
                .vertex_shader = device.get_shader<depth_only_vs>(),
                .rasterizer_state = device.get_rasterizer_state(parameter.cull_mode),
                .depth_stencil_state = device.get_depth_stencil_state(
                    true,
                    true,
                    parameter.depth_compare_op,
                    parameter.stencil_enable,
                    parameter.stencil_front,
                    parameter.stencil_back),
                .primitive_topology = parameter.primitive_topology,
            };
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            command.draw_instances(
                data.command_buffer.get_rhi(),
                data.count_buffer.get_rhi(),
                data.material_type,
                data.pipeline);
        });
}
} // namespace violet