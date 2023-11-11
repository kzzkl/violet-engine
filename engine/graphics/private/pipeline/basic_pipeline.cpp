#include "graphics/pipeline/basic_pipeline.hpp"
#include <algorithm>

namespace violet
{
basic_pipeline::basic_pipeline(std::string_view name, graphics_context* context)
    : render_pipeline(name, context)
{
}

void basic_pipeline::render(rhi_render_command* command, render_data& data)
{
    std::sort(
        data.meshes.begin(),
        data.meshes.end(),
        [](const render_mesh& a, const render_mesh& b)
        {
            return a.material < b.material;
        });

    rhi_parameter* current_material = nullptr;
    for (render_mesh& mesh : data.meshes)
    {
        command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
        command->set_index_buffer(mesh.index_buffer);
        command->set_render_parameter(0, mesh.transform);
        if (current_material != mesh.material)
        {
            command->set_render_parameter(1, mesh.material);
            current_material = mesh.material;
        }
        command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
    }
}
} // namespace violet