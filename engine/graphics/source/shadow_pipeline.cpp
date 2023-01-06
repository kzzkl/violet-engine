#include "graphics/shadow_pipeline.hpp"
#include "graphics/rhi.hpp"
#include "graphics/shadow_map.hpp"

namespace ash::graphics
{
shadow_pipeline::shadow_pipeline()
{
    render_pass_info pass_info = {};
    pass_info.vertex_shader = "engine/shader/shadow.vert";
    pass_info.vertex_attributes = {
        {"POSITION", VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // position
    };
    pass_info.references = {
        {ATTACHMENT_REFERENCE_TYPE_DEPTH, 0}
    };
    pass_info.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pass_info.parameters = {"ash_object", "ash_shadow"};
    pass_info.samples = 1;
    pass_info.rasterizer.cull_mode = CULL_MODE_BACK;
    pass_info.depth_stencil.depth_functor = DEPTH_FUNCTOR_LESS_EQUAL;

    attachment_info depth = {};
    depth.type = ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth.format = RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth.load_op = ATTACHMENT_LOAD_OP_LOAD;
    depth.store_op = ATTACHMENT_STORE_OP_STORE;
    depth.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    depth.samples = 1;
    depth.initial_state = RESOURCE_STATE_DEPTH_STENCIL;
    depth.final_state = RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    render_pipeline_info pipeline_info;
    pipeline_info.attachments.push_back(depth);
    pipeline_info.passes.push_back(pass_info);

    m_interface = rhi::make_render_pipeline(pipeline_info);
}

void shadow_pipeline::render(const render_context& context, render_command_interface* command)
{
    command->begin(m_interface.get(), nullptr, nullptr, context.shadow_map->depth_buffer());

    command->clear_depth_stencil(context.shadow_map->depth_buffer());

    scissor_extent extent = {};
    auto [width, height] = context.shadow_map->depth_buffer()->extent();
    extent.max_x = width;
    extent.max_y = height;
    command->scissor(&extent, 1);

    command->parameter(1, context.shadow_map->parameter());
    for (auto& item : context.items)
    {
        command->parameter(0, item.object_parameter);

        command->input_assembly_state(item.vertex_buffers, 1, item.index_buffer);
        command->draw_indexed(item.index_start, item.index_end, item.vertex_base);
    }

    command->end(m_interface.get());
}
} // namespace ash::graphics