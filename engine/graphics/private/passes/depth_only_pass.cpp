#include "graphics/passes/depth_only_pass.hpp"

namespace violet
{
struct depth_only_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/depth_only.hlsl";

    static constexpr input_layout inputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
    };
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

            data.pipeline = {
                .vertex_shader = render_device::instance().get_shader<depth_only_vs>(),
                .depth_stencil =
                    {
                        .depth_enable = true,
                        .depth_write_enable = true,
                        .depth_compare_op = parameter.depth_compare_op,
                        .stencil_enable = parameter.stencil_enable,
                        .stencil_front = parameter.stencil_front,
                        .stencil_back = parameter.stencil_back,
                    },
                .rasterizer =
                    {
                        .cull_mode = parameter.cull_mode,
                    },
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