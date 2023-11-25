#include "graphics/pipeline/debug_pipeline.hpp"

namespace violet
{
debug_pipeline::debug_pipeline(std::string_view name, graphics_context* context)
    : render_pipeline(name, context)
{
    set_shader("engine/shaders/debug.vert.spv", "engine/shaders/debug.frag.spv");
    set_primitive_topology(RHI_PRIMITIVE_TOPOLOGY_LINE_LIST);
    set_parameter_layouts({
        {context->get_parameter_layout("violet camera"), RENDER_PIPELINE_PARAMETER_TYPE_CAMERA}
    });
    set_vertex_attributes({
        {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
        {"color",    RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
    });
}

void debug_pipeline::render(rhi_render_command* command, render_data& data)
{
    command->set_render_parameter(0, data.camera_parameter);
    for (render_mesh& mesh : data.meshes)
    {
        command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
        command->draw(mesh.vertex_start, mesh.vertex_count);
    }
}
} // namespace violet