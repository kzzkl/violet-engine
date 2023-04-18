#include "graphics/rhi/render_pipeline.hpp"

namespace violet
{
void render_pipeline::add_mesh(mesh* mesh)
{
    m_meshes.push_back(mesh);
}

void render_pipeline::reset()
{
    m_meshes.clear();
}

void render_pipeline::render(render_command_interface* command)
{
    on_render(m_meshes, command);
}
} // namespace violet