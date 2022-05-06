#include "graphics_interface_helper.hpp"

namespace ash::graphics
{
vertex_layout_desc vertex_layout_info::convert() noexcept
{
    vertex_layout_desc result;
    result.attributes = attributes.data();
    result.attribute_count = attributes.size();
    return result;
}

pass_parameter_layout_desc pass_parameter_layout_info::convert() noexcept
{
    pass_parameter_layout_desc result;
    result.parameters = parameters.data();
    result.size = parameters.size();
    return result;
}

pass_layout_desc pass_layout_info::convert() noexcept
{
    pass_layout_desc result;
    result.parameters = nullptr;
    result.size = parameters.size();
    return result;
}

pass_blend_info::pass_blend_info()
{
    enable = false;
}

pass_info::pass_info() : primitive_topology(primitive_topology::TRIANGLE_LIST)
{
}

pass_desc pass_info::convert() noexcept
{
    pass_desc result;
    result.vertex_shader = vertex_shader.c_str();
    result.pixel_shader = pixel_shader.c_str();

    result.vertex_layout = vertex_layout.convert();

    // The pass_layout field needs to be filled in by the graphics module
    result.pass_layout = pass_layout;

    result.blend = blend.convert();
    result.depth_stencil = depth_stencil.convert();

    result.references = references.data();
    result.reference_count = references.size();

    result.primitive_topology = primitive_topology;
    result.samples = samples;

    return result;
}

technique_desc technique_info::convert() noexcept
{
    for (auto& attachment : attachments)
        m_attachment_desc.push_back(attachment.convert());

    for (auto& pass_info : subpasses)
        m_pass_desc.push_back(pass_info.convert());

    technique_desc result;
    result.attachments = m_attachment_desc.data();
    result.attachment_count = m_attachment_desc.size();
    result.subpasses = m_pass_desc.data();
    result.subpass_count = m_pass_desc.size();
    return result;
}

attachment_set_desc attachment_set_info::convert() noexcept
{
    attachment_set_desc result;
    result.attachments = attachments.data();
    result.attachment_count = attachments.size();
    result.width = width;
    result.height = height;
    result.technique = technique;
    return result;
}
} // namespace ash::graphics