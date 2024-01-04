#include "graphics/pipeline/debug_pipeline.hpp"

namespace violet
{
debug_pipeline::debug_pipeline()
{
}

bool debug_pipeline::compile(compile_context& context)
{
    set_shader("engine/shaders/debug.vert.spv", "engine/shaders/debug.frag.spv");
    set_primitive_topology(RHI_PRIMITIVE_TOPOLOGY_LINE_LIST);
    set_parameter_layouts({
        {context.renderer->get_parameter_layout("violet camera"),
         RENDER_PIPELINE_PARAMETER_TYPE_CAMERA}
    });
    set_vertex_attributes({
        {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
        {"color",    RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
    });

    return render_pipeline::compile(context);
}

void debug_pipeline::render(std::vector<render_mesh>& meshes, const execute_context& context)
{
    context.command->set_render_parameter(0, context.camera);
    for (render_mesh& mesh : meshes)
    {
        context.command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
        context.command->draw(mesh.vertex_start, mesh.vertex_count);
    }
}
} // namespace violet