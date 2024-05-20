#include "graphics/pipeline/basic_pipeline.hpp"
#include <algorithm>

namespace violet
{
basic_pipeline::basic_pipeline()
{
}

void basic_pipeline::render(std::vector<render_mesh>& meshes, const execute_context& context)
{
    rhi_parameter* current_material = nullptr;
    for (render_mesh& mesh : meshes)
    {
        context.command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
        context.command->set_index_buffer(mesh.index_buffer);
        context.command->set_render_parameter(0, mesh.transform);
        if (current_material != mesh.material)
        {
            context.command->set_render_parameter(1, mesh.material);
            current_material = mesh.material;
        }
        context.command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
    }
}
} // namespace violet