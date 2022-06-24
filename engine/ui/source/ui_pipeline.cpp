#include "ui/ui_pipeline.hpp"
#include "graphics/rhi.hpp"

namespace ash::ui
{
mvp_pipeline_parameter::mvp_pipeline_parameter() : graphics::pipeline_parameter("ui_mvp")
{
}

void mvp_pipeline_parameter::mvp_matrix(const math::float4x4& mvp)
{
    field<math::float4x4>(0) = math::matrix_plain::transpose(mvp);
}

std::vector<graphics::pipeline_parameter_pair> mvp_pipeline_parameter::layout()
{
    return {
        {graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(math::float4x4)}
    };
}

offset_pipeline_parameter::offset_pipeline_parameter() : graphics::pipeline_parameter("ui_offset")
{
}

void offset_pipeline_parameter::offset(const std::vector<math::float4>& offset)
{
    void* pointer = field_pointer(0);
    std::memcpy(pointer, offset.data(), offset.size() * sizeof(math::float4));
}

std::vector<graphics::pipeline_parameter_pair> offset_pipeline_parameter::layout()
{
    return {
        {graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, 1024 * sizeof(math::float4)}
    };
}

material_pipeline_parameter::material_pipeline_parameter()
    : graphics::pipeline_parameter("ui_material")
{
}

void material_pipeline_parameter::mesh_type(element_mesh_type type)
{
    field<std::uint32_t>(0) = static_cast<std::uint32_t>(type);
}

void material_pipeline_parameter::texture(graphics::resource_interface* texture)
{
    interface()->set(1, texture);
}

std::vector<graphics::pipeline_parameter_pair> material_pipeline_parameter::layout()
{
    return {
        {graphics::PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(std::uint32_t)}, // type
        {graphics::PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE, 1                    }  // texture
    };
}

ui_pipeline::ui_pipeline()
{
    // UI pass.
    graphics::render_pass_info ui_pass_info = {};
    ui_pass_info.vertex_shader = "engine/shader/ui.vert";
    ui_pass_info.pixel_shader = "engine/shader/ui.frag";
    ui_pass_info.vertex_attributes = {
        {"POSITION",     graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT2}, // position
        {"UV",           graphics::VERTEX_ATTRIBUTE_TYPE_FLOAT2}, // uv
        {"COLOR",        graphics::VERTEX_ATTRIBUTE_TYPE_COLOR }, // color
        {"OFFSET_INDEX", graphics::VERTEX_ATTRIBUTE_TYPE_UINT  }  // offset index
    };
    ui_pass_info.references = {
        {graphics::ATTACHMENT_REFERENCE_TYPE_COLOR,   0},
        {graphics::ATTACHMENT_REFERENCE_TYPE_DEPTH,   0},
        {graphics::ATTACHMENT_REFERENCE_TYPE_RESOLVE, 0}
    };
    ui_pass_info.primitive_topology = graphics::PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ui_pass_info.parameters = {"ui_material", "ui_offset", "ui_mvp"};
    ui_pass_info.samples = 4;
    ui_pass_info.depth_stencil.depth_functor = graphics::DEPTH_FUNCTOR_LESS;
    ui_pass_info.blend.enable = true;
    ui_pass_info.blend.source_factor = graphics::BLEND_FACTOR_SOURCE_ALPHA;
    ui_pass_info.blend.target_factor = graphics::BLEND_FACTOR_SOURCE_INV_ALPHA;
    ui_pass_info.blend.op = graphics::BLEND_OP_ADD;
    ui_pass_info.blend.source_alpha_factor = graphics::BLEND_FACTOR_SOURCE_ALPHA;
    ui_pass_info.blend.target_alpha_factor = graphics::BLEND_FACTOR_SOURCE_INV_ALPHA;
    ui_pass_info.blend.alpha_op = graphics::BLEND_OP_ADD;

    // Attachment.
    graphics::attachment_info render_target = {};
    render_target.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET;
    render_target.format = graphics::rhi::back_buffer_format();
    render_target.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    render_target.store_op = graphics::ATTACHMENT_STORE_OP_STORE;
    render_target.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    render_target.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    render_target.samples = 4;
    render_target.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    render_target.final_state = graphics::RESOURCE_STATE_RENDER_TARGET;

    graphics::attachment_info depth_stencil = {};
    depth_stencil.type = graphics::ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL;
    depth_stencil.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    depth_stencil.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_stencil.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    depth_stencil.samples = 4;
    depth_stencil.initial_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;
    depth_stencil.final_state = graphics::RESOURCE_STATE_DEPTH_STENCIL;

    graphics::attachment_info back_buffer = {};
    back_buffer.type = graphics::ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE;
    back_buffer.format = graphics::rhi::back_buffer_format();
    back_buffer.load_op = graphics::ATTACHMENT_LOAD_OP_CLEAR;
    back_buffer.store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.stencil_load_op = graphics::ATTACHMENT_LOAD_OP_DONT_CARE;
    back_buffer.stencil_store_op = graphics::ATTACHMENT_STORE_OP_DONT_CARE;
    back_buffer.samples = 1;
    back_buffer.initial_state = graphics::RESOURCE_STATE_RENDER_TARGET;
    back_buffer.final_state = graphics::RESOURCE_STATE_PRESENT;

    graphics::render_pipeline_info ui_pipeline_info;
    ui_pipeline_info.attachments.push_back(render_target);
    ui_pipeline_info.attachments.push_back(depth_stencil);
    ui_pipeline_info.attachments.push_back(back_buffer);
    ui_pipeline_info.passes.push_back(ui_pass_info);

    m_interface = graphics::rhi::make_render_pipeline(ui_pipeline_info);
}

void ui_pipeline::render(
    const graphics::camera& camera,
    const graphics::render_scene& scene,
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

    command->parameter(1, scene.units[0].parameters[1]); // offset
    command->parameter(2, scene.units[0].parameters[2]); // mvp
    for (auto& unit : scene.units)
    {
        command->scissor(&unit.scissor, 1);

        command->parameter(0, unit.parameters[0]); // material
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