#include "render/ui_pipeline.hpp"
#include "graphics/render_interface.hpp"

namespace violet::ui
{
mvp_pipeline_parameter::mvp_pipeline_parameter() : graphics::pipeline_parameter(layout)
{
}

void mvp_pipeline_parameter::mvp_matrix(const math::float4x4& mvp)
{
    field<math::float4x4>(0) = math::matrix::transpose(mvp);
}

offset_pipeline_parameter::offset_pipeline_parameter() : graphics::pipeline_parameter(layout)
{
}

void offset_pipeline_parameter::offset(const std::vector<math::float4>& offset)
{
    void* pointer = field_pointer(0);
    std::memcpy(pointer, offset.data(), offset.size() * sizeof(math::float4));
}

material_pipeline_parameter::material_pipeline_parameter() : graphics::pipeline_parameter(layout)
{
}

void material_pipeline_parameter::mesh_type(control_mesh_type type)
{
    field<std::uint32_t>(0) = static_cast<std::uint32_t>(type);
}

void material_pipeline_parameter::texture(graphics::rhi_resource* texture)
{
    interface()->set(1, texture);
}

ui_pipeline::ui_pipeline()
{
    m_mvp_parameter = std::make_unique<mvp_pipeline_parameter>();
    m_offset_parameter = std::make_unique<offset_pipeline_parameter>();

    graphics::render_pipeline_desc desc;

    // UI pass.
    graphics::render_pass_desc& ui_pass = desc.passes[0];
    ui_pass.vertex_shader = "engine/shader/ui.vert";
    ui_pass.pixel_shader = "engine/shader/ui.frag";

    ui_pass.vertex_attributes[0] = {"POSITION", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT2}; // position
    ui_pass.vertex_attributes[1] = {"UV", graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT2};       // uv
    ui_pass.vertex_attributes[2] = {"COLOR", graphics::VERTEX_ATTRIBUTE_TYPE_COLOR};     // color
    ui_pass.vertex_attributes[3] = {
        "OFFSET_INDEX",
        graphics::VERTEX_ATTRIBUTE_TYPE_UINT}; // offset index
    ui_pass.vertex_attribute_count = 4;

    ui_pass.references[0] = {graphics::ATTACHMENT_REFERENCE_TYPE_COLOR, 0};
    ui_pass.references[1] = {graphics::ATTACHMENT_REFERENCE_TYPE_DEPTH, 0};
    ui_pass.references[2] = {graphics::ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0};
    ui_pass.reference_count = 3;

    ui_pass.parameters[0] = material_pipeline_parameter::layout;
    ui_pass.parameters[1] = offset_pipeline_parameter::layout;
    ui_pass.parameters[2] = mvp_pipeline_parameter::layout;
    ui_pass.parameter_count = 3;

    ui_pass.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ui_pass.samples = 4;
    ui_pass.depth_stencil.depth_functor = graphics::DEPTH_STENCIL_FUNCTOR_LESS;
    ui_pass.blend.enable = true;
    ui_pass.blend.source_factor = graphics::BLEND_FACTOR_SOURCE_ALPHA;
    ui_pass.blend.target_factor = graphics::BLEND_FACTOR_SOURCE_INV_ALPHA;
    ui_pass.blend.op = graphics::BLEND_OP_ADD;
    ui_pass.blend.source_alpha_factor = graphics::BLEND_FACTOR_SOURCE_ALPHA;
    ui_pass.blend.target_alpha_factor = graphics::BLEND_FACTOR_SOURCE_INV_ALPHA;
    ui_pass.blend.alpha_op = graphics::BLEND_OP_ADD;

    desc.pass_count = 1;

    // Attachment.
    graphics::attachment_desc& render_target = desc.attachments[0];
    render_target.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = graphics::rhi::back_buffer_format();
    render_target.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    render_target.store_op = graphics::ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = graphics::RESOURCE_STATE_RENDER_TARGET;

    graphics::attachment_desc& depth_stencil = desc.attachments[1];
    depth_stencil.type = graphics::ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    depth_stencil.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;
    depth_stencil.final_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;

    graphics::attachment_desc& back_buffer = desc.attachments[2];
    back_buffer.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE;
    back_buffer.format = graphics::rhi::back_buffer_format();
    back_buffer.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    back_buffer.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    back_buffer.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    back_buffer.final_state = graphics::RESOURCE_STATE_PRESENT;

    desc.attachment_count = 3;

    m_interface = graphics::rhi::make_render_pipeline(desc);
}

void ui_pipeline::set_mvp_matrix(const math::float4x4& mvp)
{
    m_mvp_parameter->mvp_matrix(mvp);
}

void ui_pipeline::set_offset(const std::vector<math::float4>& offset)
{
    m_offset_parameter->offset(offset);
}

void ui_pipeline::render(
    const graphics::render_context& context,
    graphics::rhi_render_command* command)
{
    command->begin(
        m_interface.get(),
        context.render_target,
        context.render_target_resolve,
        context.depth_stencil_buffer);

    graphics::scissor_extent extent = {};
    auto [width, height] = context.render_target->extent();
    extent.max_x = width;
    extent.max_y = height;

    command->parameter(1, m_offset_parameter->interface()); // offset
    command->parameter(2, m_mvp_parameter->interface());    // mvp
    for (auto& item : context.items)
    {
        command->scissor(&item.scissor, 1);

        command->parameter(0, item.material_parameter); // material

        command->input_assembly_state(item.vertex_buffers, 4, item.index_buffer);
        command->draw_indexed(item.index_start, item.index_end, item.vertex_base);
    }

    command->end(m_interface.get());
}
} // namespace violet::ui