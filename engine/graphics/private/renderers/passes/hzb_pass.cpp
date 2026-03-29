#include "graphics/renderers/passes/hzb_pass.hpp"
#include <format>

namespace violet
{
struct hzb_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/hzb.hlsl";

    struct constant_data
    {
        std::uint32_t src;
        std::uint32_t dst_mip0;
        std::uint32_t dst_mip1;
        std::uint32_t dst_mip2;
        std::uint32_t dst_mip3;
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
        rdg_texture_srv src;
        rdg_texture_uav dst_mips[4];
        rhi_sampler* hzb_sampler;
    };

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

    std::uint32_t level_count = parameter.hzb->get_level_count();

    for (std::uint32_t level = 0; level < level_count; level += 4)
    {
        graph.add_pass<pass_data>(
            std::format("Level {} ~ {}", level, std::min(level + 3, level_count - 1)),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                if (level == 0)
                {
                    data.src =
                        pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                }
                else
                {
                    data.src = pass.add_texture_srv(
                        parameter.hzb,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_2D,
                        level - 1,
                        1);
                }

                for (std::uint32_t i = 0; i < 4; ++i)
                {
                    if (level + i < level_count)
                    {
                        data.dst_mips[i] = pass.add_texture_uav(
                            parameter.hzb,
                            RHI_PIPELINE_STAGE_COMPUTE,
                            RHI_TEXTURE_DIMENSION_2D,
                            level + i,
                            1);
                    }
                    else
                    {
                        data.dst_mips[i].reset();
                    }
                }

                data.hzb_sampler = hzb_sampler;
            },
            [](const pass_data& data, rdg_command& command)
            {
                command.set_pipeline({
                    .compute_shader = render_device::instance().get_shader<hzb_cs>(),
                });
                command.set_constant(
                    hzb_cs::constant_data{
                        .src = data.src.get_bindless(),
                        .dst_mip0 = data.dst_mips[0].get_bindless(),
                        .dst_mip1 = data.dst_mips[1] ? data.dst_mips[1].get_bindless() : 0,
                        .dst_mip2 = data.dst_mips[2] ? data.dst_mips[2].get_bindless() : 0,
                        .dst_mip3 = data.dst_mips[3] ? data.dst_mips[3].get_bindless() : 0,
                        .hzb_sampler = data.hzb_sampler->get_bindless(),
                    });
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                rhi_extent extent = data.dst_mips[0].get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }
}
} // namespace violet