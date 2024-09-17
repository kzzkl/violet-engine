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

    auto& mesh_pass_data = graph.allocate_data<mesh_pass::data>();
    mesh_pass_data.render_list = context.get_render_list(camera);
    mesh_pass_data.viewport = camera.viewport;
    mesh_pass_data.render_target = m_render_target;
    mesh_pass_data.depth_buffer = m_depth_buffer;
    mesh_pass_data.clear = true;
    graph.add_pass<mesh_pass>("Mesh Pass", mesh_pass_data);

    auto& skybox_pass_data = graph.allocate_data<skybox_pass::data>();
    skybox_pass_data.camera = camera.parameter;
    skybox_pass_data.viewport = camera.viewport;
    skybox_pass_data.render_target = m_render_target;
    skybox_pass_data.depth_buffer = m_depth_buffer;
    graph.add_pass<skybox_pass>("Skybox Pass", skybox_pass_data);
}
} // namespace violet