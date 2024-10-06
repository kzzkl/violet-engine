#include "graphics/renderers/default_renderer.hpp"
#include "graphics/passes/mesh_pass.hpp"
#include "graphics/passes/skybox_pass.hpp"

namespace violet
{
void default_renderer::render(
    render_graph& graph,
    const render_context& context,
    const render_camera& camera)
{
    m_render_target = graph.add_texture(
        "Render Target",
        camera.render_targets[0],
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_PRESENT);
    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        camera.render_targets[1],
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

    mesh_pass::add(
        graph,
        {.render_list = context.get_render_list(camera),
         .viewport = camera.viewport,
         .render_target = m_render_target,
         .depth_buffer = m_depth_buffer,
         .clear = true});

    skybox_pass::add(
        graph,
        {.camera = camera.parameter,
         .viewport = camera.viewport,
         .render_target = m_render_target,
         .depth_buffer = m_depth_buffer,
         .clear = false});
}
} // namespace violet