#include "ui/ui_pipeline.hpp"
#include "graphics/graphics.hpp"

namespace ash::ui
{
ui_pipeline::ui_pipeline()
{
    auto& graphics = system<graphics::graphics>();

    graphics::pipeline_parameter_layout_info ui_material;
    ui_material.parameters = {
        {graphics::pipeline_parameter_type::UINT,            1}, // type: 0: font, 1: image
        {graphics::pipeline_parameter_type::SHADER_RESOURCE, 1}  // texture
    };
    graphics.make_pipeline_parameter_layout("ui_material", ui_material);
    graphics::pipeline_parameter_layout_info ui_offset;
    ui_offset.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4_ARRAY, 2048}, // offset
    };
    graphics.make_pipeline_parameter_layout("ui_offset", ui_offset);

    graphics::pipeline_parameter_layout_info ui_mvp;
    ui_mvp.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4x4, 1}, // mvp
    };
    graphics.make_pipeline_parameter_layout("ui_mvp", ui_mvp);

    // UI pass.
    graphics::pipeline_info ui_pipeline_info = {};
    ui_pipeline_info.vertex_shader = "engine/shader/ui.vert";
    ui_pipeline_info.pixel_shader = "engine/shader/ui.frag";
    ui_pipeline_info.vertex_attributes = {
        {"POSITION",     graphics::vertex_attribute_type::FLOAT2}, // position
        {"UV",           graphics::vertex_attribute_type::FLOAT2}, // uv
        {"COLOR",        graphics::vertex_attribute_type::COLOR }, // color
        {"OFFSET_INDEX", graphics::vertex_attribute_type::UINT  }  // offset index
    };
    ui_pipeline_info.references = {
        {graphics::attachment_reference_type::COLOR,   0},
        {graphics::attachment_reference_type::DEPTH,   0},
        {graphics::attachment_reference_type::RESOLVE, 0}
    };
    ui_pipeline_info.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ui_pipeline_info.parameters = {"ui_material", "ui_offset", "ui_mvp"};
    ui_pipeline_info.samples = 4;
    ui_pipeline_info.depth_stencil.depth_functor = graphics::depth_functor::LESS;
    ui_pipeline_info.blend.enable = true;
    ui_pipeline_info.blend.source_factor = graphics::blend_factor::SOURCE_ALPHA;
    ui_pipeline_info.blend.target_factor = graphics::blend_factor::SOURCE_INV_ALPHA;
    ui_pipeline_info.blend.op = graphics::blend_op::ADD;
    ui_pipeline_info.blend.source_alpha_factor = graphics::blend_factor::SOURCE_ALPHA;
    ui_pipeline_info.blend.target_alpha_factor = graphics::blend_factor::SOURCE_INV_ALPHA;
    ui_pipeline_info.blend.alpha_op = graphics::blend_op::ADD;

    // Attachment.
    graphics::attachment_info render_target = {};
    render_target.type = graphics::attachment_type::CAMERA_RENDER_TARGET;
    render_target.format = graphics.back_buffer_format();
    render_target.load_op = graphics::attachment_load_op::CLEAR;
    render_target.store_op = graphics::attachment_store_op::STORE;
    render_target.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    render_target.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::resource_state::RENDER_TARGET;
    render_target.final_state = graphics::resource_state::RENDER_TARGET;

    graphics::attachment_info depth_stencil = {};
    depth_stencil.type = graphics::attachment_type::CAMERA_DEPTH_STENCIL;
    depth_stencil.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::attachment_load_op::CLEAR;
    depth_stencil.store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    depth_stencil.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::resource_state::DEPTH_STENCIL;
    depth_stencil.final_state = graphics::resource_state::DEPTH_STENCIL;

    graphics::attachment_info back_buffer = {};
    back_buffer.type = graphics::attachment_type::CAMERA_RENDER_TARGET_RESOLVE;
    back_buffer.format = graphics.back_buffer_format();
    back_buffer.load_op = graphics::attachment_load_op::CLEAR;
    back_buffer.store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.stencil_load_op = graphics::attachment_load_op::DONT_CARE;
    back_buffer.stencil_store_op = graphics::attachment_store_op::DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::resource_state::RENDER_TARGET;
    back_buffer.final_state = graphics::resource_state::PRESENT;

    graphics::render_pass_info ui_pass_info;
    ui_pass_info.attachments.push_back(render_target);
    ui_pass_info.attachments.push_back(depth_stencil);
    ui_pass_info.attachments.push_back(back_buffer);
    ui_pass_info.subpasses.push_back(ui_pipeline_info);

    m_interface = graphics.make_render_pass(ui_pass_info);
}

void ui_pipeline::render(
    const graphics::camera& camera,
    graphics::render_command_interface* command)
{
    command->begin(
        m_interface.get(),
        camera.render_target(),
        camera.render_target_resolve(),
        camera.depth_stencil_buffer());

    graphics::scissor_extent extent = {};
    auto [width, height] = camera.render_target()->extent();
    extent.max_x = width;
    extent.max_y = height;

    command->parameter(1, units()[0].parameters[1]->interface()); // offset
    command->parameter(2, units()[0].parameters[2]->interface()); // mvp
    for (auto& unit : units())
    {
        command->scissor(&unit.scissor, 1);

        command->parameter(0, unit.parameters[0]->interface()); // material
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
} // namespace ash::ui