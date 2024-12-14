#include "mmd_renderer.hpp"
#include "graphics/passes/blit_pass.hpp"

namespace violet::sample
{
void mmd_renderer::render(
    render_graph& graph,
    const render_scene& scene,
    const render_camera& camera)
{
    m_render_extent = camera.render_targets[0]->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_R16G16B16A16_FLOAT,
            .flags =
                RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_SRC,
        });

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        {
            .extent = m_render_extent,
            .format = RHI_FORMAT_D32_FLOAT_S8_UINT,
            .flags = RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE,
        });

    add_present_pass(graph, camera);
}

void mmd_renderer::add_skinning_pass(render_graph& graph) {}

void mmd_renderer::add_present_pass(render_graph& graph, const render_camera& camera)
{
    rdg_texture* camera_output = graph.add_texture(
        "Camera Output",
        camera.render_targets[0],
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

    blit_pass::add(
        graph,
        {
            .src = m_render_target,
            .src_region = region,
            .dst = camera_output,
            .dst_region = region,
        });
}
} // namespace violet::sample