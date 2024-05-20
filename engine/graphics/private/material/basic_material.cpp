#include "graphics/material/basic_material.hpp"
#include "core/context/engine.hpp"
#include "graphics/graphics_module.hpp"

namespace violet
{
basic_material::basic_material() : parameter(layout)
{
}

void basic_material::set_color(const float3& color)
{
    m_data.color = color;
    get_field<constant_data>(0).color = color;
}

const float3& basic_material::get_color() const noexcept
{
    return m_data.color;
}

basic_pipeline::basic_pipeline() : render_pipeline({"position", "normal"})
{
    auto& graphics = engine::get_module<graphics_module>();

    render_pipeline_desc desc = {};

    auto& color_pass = desc.passes[0];
    color_pass.vertex_attributes[0].name = "POSITION";
    color_pass.vertex_attributes[0].type = vertex_attribute_type::VERTEX_ATTRIBUTE_TYPE_FLOAT3;
    color_pass.vertex_attribute_count = 1;
    color_pass.references[0] = {ATTACHMENT_REFERENCE_TYPE_COLOR, 0};
    color_pass.references[1] = {ATTACHMENT_REFERENCE_TYPE_DEPTH, 0};
    color_pass.references[2] = {ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0};
    color_pass.reference_count = 3;
    color_pass.parameters[0] = basic_material::layout;
    color_pass.parameter_count = 1;
    color_pass.primitive_topology = PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.pass_count = 1;

    attachment_desc& render_target = desc.attachments[0];
    render_target.type = ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = engine_graphics.get_rhi()->get_back_buffer()->get_format();
    render_target.load_op = ATTACHMENT_LOAD_OP_LOAD;
    render_target.store_op = ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = RESOURCE_STATE_RENDER_TARGET;

    attachment_desc& depth_stencil = desc.attachments[1];
    depth_stencil.type = ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = ATTACHMENT_LOAD_OP_LOAD;
    depth_stencil.store_op = ATTACHMENT_STORE_OP_STORE;
    depth_stencil.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = RESOURCE_STATE_UNDEFINED;
    depth_stencil.final_state = RESOURCE_STATE_UNDEFINED;

    attachment_desc& render_target_resolve = desc.attachments[2];
    render_target_resolve.type = ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE;
    render_target_resolve.format = resource_format::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    render_target_resolve.load_op = ATTACHMENT_LOAD_OP_CLEAR;
    render_target_resolve.store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target_resolve.stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target_resolve.stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
    render_target_resolve.samples = 1;
    render_target_resolve.initial_state = RESOURCE_STATE_RENDER_TARGET;
    render_target_resolve.final_state = RESOURCE_STATE_PRESENT;

    m_interface.reset(engine_graphics.get_rhi()->create_render_pipeline(desc));
}

void basic_pipeline::on_render(const std::vector<mesh*>& meshes, rhi_command* command)
{
}
} // namespace violet