#include "mmd_pipeline.hpp"

namespace ash::sample::mmd
{
mmd_pass::mmd_pass()
{
    auto& graphics = system<graphics::graphics>();

    graphics::pipeline_layout_info mmd_material;
    mmd_material.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4,  1}, // diffuse
        {graphics::pipeline_parameter_type::FLOAT3,  1}, // specular
        {graphics::pipeline_parameter_type::FLOAT,   1}, // specular_strength
        {graphics::pipeline_parameter_type::UINT,    1}, // toon_mode
        {graphics::pipeline_parameter_type::UINT,    1}, // spa_mode
        {graphics::pipeline_parameter_type::TEXTURE, 1}, // tex
        {graphics::pipeline_parameter_type::TEXTURE, 1}, // toon
        {graphics::pipeline_parameter_type::TEXTURE, 1}  // spa
    };
    graphics.make_pipeline_layout("mmd_material", mmd_material);

    graphics::pipeline_layout_info mmd_skeleton;
    mmd_skeleton.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4x4_ARRAY, 512}, // offset
    };
    graphics.make_pipeline_layout("mmd_skeleton", mmd_skeleton);

    // Pass.
    graphics::pipeline_info color_pass_info = {};
    color_pass_info.vertex_shader = "resource/shader/glsl/vert.spv";
    color_pass_info.pixel_shader = "resource/shader/glsl/frag.spv";
    color_pass_info.vertex_attributes = {
        graphics::vertex_attribute_type::FLOAT3, // position
        graphics::vertex_attribute_type::FLOAT3, // normal
        graphics::vertex_attribute_type::FLOAT2, // uv
        graphics::vertex_attribute_type::UINT4,  // bone
        graphics::vertex_attribute_type::FLOAT3, // bone weight
    };
    color_pass_info.references = {
        {graphics::attachment_reference_type::COLOR,   0},
        {graphics::attachment_reference_type::DEPTH,   0},
        {graphics::attachment_reference_type::RESOLVE, 0}
    };
    color_pass_info.primitive_topology = graphics::primitive_topology::TRIANGLE_LIST;
    color_pass_info.parameters = {"ash_object", "mmd_material", "mmd_skeleton", "ash_pass"};
    color_pass_info.samples = 4;

    // Attachment.
    graphics::attachment_info render_target = {};
    render_target.type = graphics::attachment_type::COLOR;
    render_target.format = graphics.back_buffers()[0]->format();
    render_target.load_op = graphics::attachment_load_op::CLEAR;
    render_target.store_op = graphics::attachment_store_op::STORE;
    render_target.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    render_target.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::resource_state::UNDEFINED;
    render_target.final_state = graphics::resource_state::COLOR;

    graphics::attachment_info depth_stencil = {};
    depth_stencil.type = graphics::attachment_type::DEPTH;
    depth_stencil.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::attachment_load_op::CLEAR;
    depth_stencil.store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    depth_stencil.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::resource_state::UNDEFINED;
    depth_stencil.final_state = graphics::resource_state::DEPTH_STENCIL;

    graphics::attachment_info back_buffer = {};
    back_buffer.type = graphics::attachment_type::RENDER_TARGET;
    back_buffer.format = graphics.back_buffers()[0]->format();
    back_buffer.load_op = graphics::attachment_load_op::CLEAR;
    back_buffer.store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    back_buffer.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::resource_state::UNDEFINED;
    back_buffer.final_state = graphics::resource_state::PRESENT;

    graphics::render_pass_info mmd_render_pass_info;
    mmd_render_pass_info.attachments.push_back(render_target);
    mmd_render_pass_info.attachments.push_back(depth_stencil);
    mmd_render_pass_info.attachments.push_back(back_buffer);
    mmd_render_pass_info.subpasses.push_back(color_pass_info);

    m_interface = graphics.make_render_pass(mmd_render_pass_info);
}

void mmd_pass::render(const graphics::camera& camera, graphics::render_command_interface* command)
{
    command->begin(m_interface.get(), camera.render_target);

    graphics::scissor_rect rect = {};
    rect.max_x = camera.render_target->width();
    rect.max_y = camera.render_target->height();
    command->scissor(rect);

    command->parameter(3, camera.parameter->parameter());
    for (auto& unit : units())
    {
        command->parameter(0, unit->parameters[0]->parameter());
        command->parameter(1, unit->parameters[1]->parameter());
        command->parameter(2, unit->parameters[2]->parameter());

        command->draw(
            unit->vertex_buffer,
            unit->index_buffer,
            unit->index_start,
            unit->index_end,
            unit->vertex_base);
    }

    command->end(m_interface.get());
}
} // namespace ash::sample::mmd