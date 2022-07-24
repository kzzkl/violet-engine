#include "graphics/shadow_pipeline.hpp"
#include "graphics/rhi.hpp"

namespace ash::graphics
{
shadow_pipeline::shadow_pipeline()
{
    render_pass_info pass_info = {};
    pass_info.vertex_shader = "engine/shader/shadow.vert";
    pass_info.pixel_shader = "engine/shader/shadow.frag";
    pass_info.vertex_attributes = {
        {"POSITION", VERTEX_ATTRIBUTE_TYPE_FLOAT3}, // position
    };
    pass_info.references = {
        {ATTACHMENT_REFERENCE_TYPE_COLOR, 0},
        {ATTACHMENT_REFERENCE_TYPE_DEPTH, 0}
    };
    pass_info.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pass_info.parameters = {"ash_object", "ash_shadow"};
    pass_info.samples = 4;
    pass_info.rasterizer.cull_mode = CULL_MODE_BACK;
    pass_info.depth_stencil.depth_functor = DEPTH_FUNCTOR_LESS_EQUAL;

    attachment_info render_target = {};
    render_target.type = ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = rhi::back_buffer_format();
    render_target.load_op = ATTACHMENT_LOAD_OP_LOAD;
    render_target.store_op = ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = RESOURCE_STATE_RENDER_TARGET;

    attachment_info depth = {};
    depth.type = ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth.format = RESOURCE_FORMAT_D32_FLOAT;
    depth.load_op = ATTACHMENT_LOAD_OP_LOAD;
    depth.store_op = ATTACHMENT_STORE_OP_STORE;
    depth.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    depth.samples = 4;
    depth.initial_state = RESOURCE_STATE_DEPTH_STENCIL;
    depth.final_state = RESOURCE_STATE_DEPTH_STENCIL;

    render_pipeline_info pipeline_info;
    pipeline_info.attachments.push_back(render_target);
    pipeline_info.attachments.push_back(depth);
    pipeline_info.passes.push_back(pass_info);

    m_interface = rhi::make_render_pipeline(pipeline_info);
}

void shadow_pipeline::on_render(const render_scene& scene, render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        scene.shadow_map->m_test.get(),
        nullptr,
        scene.shadow_map->depth_buffer());

    scissor_extent extent = {};
    auto [width, height] = scene.shadow_map->depth_buffer()->extent();
    extent.max_x = width;
    extent.max_y = height;
    command->scissor(&extent, 1);

    command->parameter(1, scene.shadow_map->parameter());
    for (auto& item : scene.items)
    {
        command->parameter(0, item.object_parameter);

        command->input_assembly_state(item.vertex_buffers, 1, item.index_buffer);
        command->draw_indexed(item.index_start, item.index_end, item.vertex_base);
    }
}
} // namespace ash::graphics