#include "ui_pipeline.hpp"
#include "graphics.hpp"

namespace ash::ui
{
ui_pass::ui_pass()
{
    auto& graphics = system<graphics::graphics>();

    graphics::pipeline_parameter_layout_info ui_material;
    ui_material.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4x4, 1}, // ui_mvp
        {graphics::pipeline_parameter_type::TEXTURE,  1}  // ui_texture
    };
    graphics.make_pipeline_parameter_layout("ui_material", ui_material);

    // UI pass.
    graphics::pipeline_info ui_pass_info = {};
    ui_pass_info.vertex_shader = "engine/shader/ui.vert";
    ui_pass_info.pixel_shader = "engine/shader/ui.frag";
    ui_pass_info.vertex_attributes = {
        {"POSITION", graphics::vertex_attribute_type::FLOAT2}, // position
        {"UV",       graphics::vertex_attribute_type::FLOAT2}, // uv
        {"COLOR",    graphics::vertex_attribute_type::COLOR }  // normal
    };
    ui_pass_info.references = {
        {graphics::attachment_reference_type::COLOR, 0},
        {graphics::attachment_reference_type::DEPTH, 0}
    };
    ui_pass_info.primitive_topology = graphics::primitive_topology::TRIANGLE_LIST;
    ui_pass_info.parameters = {"ui_material"};
    ui_pass_info.samples = 4;

    // Attachment.
    graphics::attachment_info render_target = {};
    render_target.type = graphics::attachment_type::RENDER_TARGET;
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
}

void ui_pass::render(const graphics::camera& camera, graphics::render_command_interface* command)
{
    /*command->pipeline(pipeline());
    command->layout(layout());
    command->render_target(target, depth_stencil);

    for (auto& unit : units())
    {
        for (std::size_t i = 0; i < unit_parameter_count(); ++i)
            command->parameter(i, unit->parameters[i]->parameter());

        auto rect = static_cast<graphics::scissor_rect*>(unit->external);
        command->scissor(*rect);
        command->draw(
            unit->vertex_buffer,
            unit->index_buffer,
            unit->index_start,
            unit->index_end,
            unit->vertex_base,
            graphics::primitive_topology_type::TRIANGLE_LIST,
            target);
    }*/
}
} // namespace ash::ui