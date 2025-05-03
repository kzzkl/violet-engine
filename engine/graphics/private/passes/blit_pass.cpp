#include "graphics/passes/blit_pass.hpp"

namespace violet
{
void blit_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_ref src;
        rhi_texture_region src_region;
        rdg_texture_ref dst;
        rhi_texture_region dst_region;
        rhi_filter filter;
    };

    graph.add_pass<pass_data>(
        "Blit Pass",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.src = pass.add_texture(
                parameter.src,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_READ,
                RHI_TEXTURE_LAYOUT_TRANSFER_SRC);
            data.src_region = parameter.src_region;

            data.dst = pass.add_texture(
                parameter.dst,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE,
                RHI_TEXTURE_LAYOUT_TRANSFER_DST);
            data.dst_region = parameter.dst_region;

            data.filter = parameter.filter;
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.blit_texture(
                data.src.get_rhi(),
                data.src_region,
                data.dst.get_rhi(),
                data.dst_region,
                data.filter);
        });
}
} // namespace violet