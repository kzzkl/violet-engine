#include "graphics/render_pipeline.hpp"

namespace ash::graphics
{
render_pipeline::render_pipeline()
{
}

void render_pipeline::add(const visual& visual, std::size_t submesh_index)
{
    m_units.push_back(render_unit{
        .vertex_buffers = visual.vertex_buffers,
        .index_buffer = visual.index_buffer,
        .index_start = visual.submeshes[submesh_index].index_start,
        .index_end = visual.submeshes[submesh_index].index_end,
        .vertex_base = visual.submeshes[submesh_index].vertex_base,
        .parameters = visual.materials[submesh_index].parameters,
        .scissor = visual.materials[submesh_index].scissor});
}
} // namespace ash::graphics