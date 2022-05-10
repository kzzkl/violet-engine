#include "graphics_interface_helper.hpp"

namespace ash::graphics
{
pipeline_parameter_layout_desc pipeline_parameter_layout_info::convert() noexcept
{
    pipeline_parameter_layout_desc result;
    result.parameters = parameters.data();
    result.size = parameters.size();
    return result;
}

blend_info::blend_info()
{
    enable = false;
}

pipeline_info::pipeline_info() : primitive_topology(primitive_topology::TRIANGLE_LIST)
{
}

pipeline_desc pipeline_info::convert() noexcept
{
    pipeline_desc result;
    result.vertex_shader = vertex_shader.c_str();
    result.pixel_shader = pixel_shader.c_str();

    result.vertex_attributes = vertex_attributes.data();
    result.vertex_attribute_count = vertex_attributes.size();

    result.parameters = parameter_interfaces.data();
    result.parameter_count = parameter_interfaces.size();

    result.blend = blend.convert();
    result.depth_stencil = depth_stencil.convert();

    result.references = references.data();
    result.reference_count = references.size();

    result.primitive_topology = primitive_topology;
    result.samples = samples;

    return result;
}

render_pass_desc render_pass_info::convert() noexcept
{
    for (auto& attachment : attachments)
        m_attachment_desc.push_back(attachment.convert());

    for (auto& pass_info : subpasses)
        m_pass_desc.push_back(pass_info.convert());

    render_pass_desc result;
    result.attachments = m_attachment_desc.data();
    result.attachment_count = m_attachment_desc.size();
    result.subpasses = m_pass_desc.data();
    result.subpass_count = m_pass_desc.size();
    return result;
}
} // namespace ash::graphics