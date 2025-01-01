#include "graphics/passes/blit_pass.hpp"

namespace violet
{
void blit_pass::add(render_graph& graph, const parameter& parameter)
{
    auto& pass = graph.add_pass<rdg_pass>("Blit Pass");
    pass.add_texture(
        parameter.src,
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_ACCESS_TRANSFER_READ,
        RHI_TEXTURE_LAYOUT_TRANSFER_SRC);
    pass.add_texture(
        parameter.dst,
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_ACCESS_TRANSFER_WRITE,
        RHI_TEXTURE_LAYOUT_TRANSFER_DST);
    pass.set_execute(
        [parameter](rdg_command& command)
        {
            command.blit_texture(
                parameter.src->get_rhi(),
                parameter.src_region,
                parameter.dst->get_rhi(),
                parameter.dst_region);
        });
}
} // namespace violet