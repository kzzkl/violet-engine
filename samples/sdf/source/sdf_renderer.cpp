#include "sdf_renderer.hpp"
#include "graphics/renderers/passes/blit_pass.hpp"
#include "sdf_debug_pass.hpp"

namespace violet
{
sdf_renderer::sdf_renderer() = default;

void sdf_renderer::on_render(render_graph& graph)
{
    const auto& context = graph.get_context();

    m_render_extent = context.get_render_target()->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        m_render_extent,
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE |
            RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_TRANSFER_DST);

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        m_render_extent,
        RHI_FORMAT_D32_FLOAT,
        RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE);

    graph.add_pass<sdf_debug_pass>({
        .bounding_box = m_bounds,
        .matrix_m = m_matrix,
        .sdf = m_sdf,
        .render_target = m_render_target,
        .depth_buffer = m_depth_buffer,
        .render_bounds = m_bounds_enable,
        .render_bricks = m_brick_enable,
    });

    rdg_texture* camera_output = graph.add_texture(
        "Camera Output",
        graph.get_context().get_render_target(),
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_PRESENT);

    rhi_texture_region region = {
        .offset_x = 0,
        .offset_y = 0,
        .extent = m_render_extent,
        .level = 0,
        .layer = 0,
        .layer_count = 1,
    };

    m_imgui_pass.add(
        graph,
        {
            .render_target = m_render_target,
        });

    graph.add_pass<blit_pass>({
        .src = m_render_target,
        .src_region = region,
        .dst = camera_output,
        .dst_region = region,
    });

    m_render_target = camera_output;
}
} // namespace violet