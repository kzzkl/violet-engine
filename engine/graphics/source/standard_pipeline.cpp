#include "graphics/standard_pipeline.hpp"
#include "graphics/graphics.hpp"

namespace ash::graphics
{
standard_pipeline::standard_pipeline()
{
    pipeline_parameter_layout_info standard_material;
    standard_material.parameters = {
        {pipeline_parameter_type::FLOAT3, 1}  // diffuse
    };
    system<graphics>().make_pipeline_parameter_layout("standard_material", standard_material);

    // Color pass.
    pipeline_info color_pass_info = {};
    color_pass_info.vertex_shader = "engine/shader/standard.vert";
    color_pass_info.pixel_shader = "engine/shader/standard.frag";
    color_pass_info.vertex_attributes = {
        {"POSITION",    vertex_attribute_type::FLOAT3}, // position
        {"NORMAL",      vertex_attribute_type::FLOAT3}, // normal
    };
    color_pass_info.references = {
        {attachment_reference_type::COLOR,   0},
        {attachment_reference_type::DEPTH,   0},
        {attachment_reference_type::RESOLVE, 0}
    };
    color_pass_info.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    color_pass_info.parameters = {"ash_object", "standard_material", "ash_pass"};
    color_pass_info.samples = 4;

    // Attachment.
    attachment_info render_target = {};
    render_target.type = attachment_type::CAMERA_RENDER_TARGET;
    render_target.format = system<graphics>().back_buffer_format();
    render_target.load_op = attachment_load_op::LOAD;
    render_target.store_op = attachment_store_op::STORE;
    render_target.stencil_load_op = attachment_load_op::DONT_CARE;
    render_target.stencil_store_op = attachment_store_op::DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = resource_state::RENDER_TARGET;
    render_target.final_state = resource_state::RENDER_TARGET;

    attachment_info depth_stencil = {};
    depth_stencil.type = attachment_type::CAMERA_DEPTH_STENCIL;
    depth_stencil.format = resource_format::D24_UNORM_S8_UINT;
    depth_stencil.load_op = attachment_load_op::LOAD;
    depth_stencil.store_op = attachment_store_op::STORE;
    depth_stencil.stencil_load_op = attachment_load_op::DONT_CARE;
    depth_stencil.stencil_store_op = attachment_store_op::DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = resource_state::DEPTH_STENCIL;
    depth_stencil.final_state = resource_state::DEPTH_STENCIL;

    attachment_info render_target_resolve = {};
    render_target_resolve.type = attachment_type::CAMERA_RENDER_TARGET_RESOLVE;
    render_target_resolve.format = system<graphics>().back_buffer_format();
    render_target_resolve.load_op = attachment_load_op::CLEAR;
    render_target_resolve.store_op = attachment_store_op::DONT_CARE;
    render_target_resolve.stencil_load_op = attachment_load_op::DONT_CARE;
    render_target_resolve.stencil_store_op = attachment_store_op::DONT_CARE;
    render_target_resolve.samples = 1;
    render_target_resolve.initial_state = resource_state::RENDER_TARGET;
    render_target_resolve.final_state = resource_state::RENDER_TARGET;

    render_pass_info standard_pass_info;
    standard_pass_info.attachments.push_back(render_target);
    standard_pass_info.attachments.push_back(depth_stencil);
    standard_pass_info.attachments.push_back(render_target_resolve);
    standard_pass_info.subpasses.push_back(color_pass_info);

    m_interface = system<graphics>().make_render_pass(standard_pass_info);
}

void standard_pipeline::render(const camera& camera, render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        camera.render_target(),
        camera.render_target_resolve(),
        camera.depth_stencil_buffer());

    scissor_extent extent = {};
    auto [width, height] = camera.render_target()->extent();
    extent.max_x = width;
    extent.max_y = height;
    command->scissor(&extent, 1);

    command->parameter(2, camera.parameter()->interface());
    for (auto& unit : units())
    {
        command->parameter(0, unit.parameters[0]->interface());
        command->parameter(1, unit.parameters[1]->interface());

        command->draw(
            unit.vertex_buffers.data(),
            unit.vertex_buffers.size(),
            unit.index_buffer,
            unit.index_start,
            unit.index_end,
            unit.vertex_base);
    }

    command->end(m_interface.get());
}
} // namespace ash::graphics