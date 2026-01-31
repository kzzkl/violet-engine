#include "graphics/renderers/passes/hzb_pass.hpp"

namespace violet
{
struct hzb_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/hzb.hlsl";

    struct constant_data
    {
        std::uint32_t prev_buffer;
        std::uint32_t next_buffer;
        std::uint32_t prev_width;
        std::uint32_t prev_height;
        std::uint32_t next_width;
        std::uint32_t next_height;
        std::uint32_t level;
        std::uint32_t hzb_sampler;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
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
            },
            [](const pass_data& data, rdg_command& command)
            {
                rhi_texture_extent prev_extent = data.prev_buffer.get_extent();
                rhi_texture_extent next_extent = data.next_buffer.get_extent();

                auto* hzb_sampler = render_device::instance().get_sampler({
                    .mag_filter = RHI_FILTER_LINEAR,
                    .min_filter = RHI_FILTER_LINEAR,
                    .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
                    .min_level = 0.0f,
                    .max_level = -1.0f,
                    .reduction_mode = RHI_SAMPLER_REDUCTION_MODE_MIN,
                });

                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<hzb_cs>(),
                });
                command.set_constant(
                    hzb_cs::constant_data{
                        .prev_buffer = data.prev_buffer.get_bindless(),
                        .next_buffer = data.next_buffer.get_bindless(),
                        .prev_width = prev_extent.width,
                        .prev_height = prev_extent.height,
                        .next_width = next_extent.width,
                        .next_height = next_extent.height,
                        .level = data.level,
                        .hzb_sampler = hzb_sampler->get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                command.dispatch_2d(next_extent.width, next_extent.height);
            });
    }
}
} // namespace violet