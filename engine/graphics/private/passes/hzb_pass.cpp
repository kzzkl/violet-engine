#include "graphics/passes/hzb_pass.hpp"

namespace violet
{
struct hzb_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/hzb.hlsl";

    struct constant_data
    {
        std::uint32_t prev_buffer;
        std::uint32_t next_buffer;
        std::uint32_t level;
        std::uint32_t prev_width;
        std::uint32_t prev_height;
    };

    static constexpr parameter hzb = {
        {RHI_PARAMETER_BINDING_CONSTANT, RHI_SHADER_STAGE_COMPUTE, sizeof(constant_data)},
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, hzb},
    };
};

void hzb_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "HZB");

    struct pass_data
    {
        rdg_texture_srv prev_buffer;
        rdg_texture_uav next_buffer;
        std::uint32_t level;
        rhi_parameter* hzb_parameter;
    };

    for (std::uint32_t level = 0; level < parameter.hzb->get_level_count(); ++level)
    {
        graph.add_pass<pass_data>(
            "Level " + std::to_string(level),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                if (level == 0)
                {
                    data.prev_buffer =
                        pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                }
                else
                {
                    data.prev_buffer = pass.add_texture_srv(
                        parameter.hzb,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_2D,
                        level - 1,
                        1);
                }

                data.next_buffer = pass.add_texture_uav(
                    parameter.hzb,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
                data.level = level;

                data.hzb_parameter = pass.add_parameter(hzb_cs::hzb);
            },
            [](const pass_data& data, rdg_command& command)
            {
                rhi_texture_extent extent = data.next_buffer.get_extent();

                hzb_cs::constant_data constant = {
                    .prev_buffer = data.prev_buffer.get_bindless(),
                    .next_buffer = data.next_buffer.get_bindless(),
                    .level = data.level,
                    .prev_width = extent.width,
                    .prev_height = extent.height,
                };
                data.hzb_parameter->set_constant(0, &constant, sizeof(hzb_cs::constant_data));

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<hzb_cs>(),
                });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, data.hzb_parameter);

                command.dispatch_2d(extent.width, extent.height);
            });
    }
}
} // namespace violet