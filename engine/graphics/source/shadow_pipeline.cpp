#include "graphics/shadow_pipeline.hpp"
#include "graphics/mesh_render.hpp"
#include "graphics/rhi.hpp"
#include "graphics/shadow_map.hpp"

namespace violet::graphics
{
shadow_pipeline::shadow_pipeline()
{
    render_pipeline_desc desc;

    render_pass_desc& shadow_pass = desc.passes[0];
    shadow_pass.vertex_shader = "engine/shader/shadow.vert";
    shadow_pass.vertex_attributes[0] = {"POSITION", VERTEX_ATTRIBUTE_TYPE_FLOAT3}; // position
    shadow_pass.vertex_attribute_count = 1;

    shadow_pass.references[0] = {ATTACHMENT_REFERENCE_TYPE_DEPTH, 0};
    shadow_pass.reference_count = 1;

    shadow_pass.parameters[0] = object_pipeline_parameter::layout;
    shadow_pass.parameters[1] = shadow_map_pipeline_parameter::layout;
    shadow_pass.parameter_count = 2;

    shadow_pass.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    shadow_pass.samples = 1;
    shadow_pass.rasterizer.cull_mode = CULL_MODE_BACK;
    shadow_pass.depth_stencil.depth_functor = DEPTH_STENCIL_FUNCTOR_LESS_EQUAL;

    desc.pass_count = 1;

    attachment_desc& depth_stencil = desc.attachments[0];
    depth_stencil.type = ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = ATTACHMENT_LOAD_OP_LOAD;
    depth_stencil.store_op = ATTACHMENT_STORE_OP_STORE;
    depth_stencil.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 1;
    depth_stencil.initial_state = RESOURCE_STATE_DEPTH_STENCIL;
    depth_stencil.final_state = RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    desc.attachment_count = 1;

    m_interface = rhi::make_render_pipeline(desc);
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
} // namespace violet::graphics