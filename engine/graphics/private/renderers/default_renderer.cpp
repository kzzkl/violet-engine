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

    add_pass<mesh_pass>(graph, context, camera, m_render_target, m_depth_buffer);
    add_pass<skybox_pass>(graph, camera, m_render_target, m_depth_buffer);
}
} // namespace violet