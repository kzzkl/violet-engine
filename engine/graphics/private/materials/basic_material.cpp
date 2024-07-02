#include "graphics/materials/basic_material.hpp"

namespace violet
{
basic_material::basic_material(const float3& color)
{
    rdg_render_pipeline pipeline = {};
    pipeline.vertex_shader = render_device::instance().get_shader("engine/shaders/basic.vert.spv");
    pipeline.fragment_shader =
        render_device::instance().get_shader("engine/shaders/basic.frag.spv");
    pipeline.input.vertex_attributes[0] = {
        .name = "position",
        .format = RHI_FORMAT_R32G32B32_FLOAT};
    pipeline.input.vertex_attribute_count = 1;
    pipeline.blend.attachment_count = 1;

    rhi_parameter_desc parameter = {};
    parameter.bindings[0] = {
        RHI_PARAMETER_TYPE_UNIFORM,
        sizeof(data),
        RHI_SHADER_STAGE_FLAG_FRAGMENT};
    parameter.binding_count = 1;
    add_pass(pipeline, parameter);

    set_color(color);
}

void basic_material::set_color(const float3& color)
{
    get_parameter(0)->set_uniform(0, &color, sizeof(float3), 0);
    m_data.color = color;
}

const float3& basic_material::get_color() const noexcept
{
    return m_data.color;
}
} // namespace violet