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

pipeline_parameter_layout_desc pipeline_parameter_layout_info::convert() noexcept
{
    pipeline_parameter_layout_desc result;
    result.parameters = parameters.data();
    result.size = parameters.size();
    return result;
}

pipeline_layout_desc pipeline_layout_info::convert() noexcept
{
    pipeline_layout_desc result;
    result.parameters = nullptr;
    result.size = parameters.size();
    return result;
}

pipeline_blend_info::pipeline_blend_info()
{
    enable = false;
}

pipeline_info::pipeline_info()
    : depth(0),
      output_depth(false),
      primitive_topology(primitive_topology_type::TRIANGLE_LIST)
{
}

pipeline_desc pipeline_info::convert() noexcept
{
    pipeline_desc result;
    result.vertex_shader = vertex_shader.c_str();
    result.pixel_shader = pixel_shader.c_str();

    result.vertex_layout = vertex_layout.convert();

    // The pipeline_layout field needs to be filled in by the graphics module
    result.pipeline_layout = pipeline_layout;

    result.blend = blend.convert();
    result.depth_stencil = depth_stencil.convert();

    result.input = input.data();
    result.input_count = input.size();

    result.output = output.data();
    result.output_count = output.size();

    result.depth = depth;
    result.output_depth = output_depth;

    result.primitive_topology = primitive_topology;

    return result;
}

render_pass_desc render_pass_info::convert() noexcept
{
    for (auto& render_target : render_targets)
        m_render_target_desc.push_back(render_target.convert());

    for (auto& pass_info : subpasses)
        m_pass_desc.push_back(pass_info.convert());

    render_pass_desc result;
    result.render_targets = m_render_target_desc.data();
    result.render_target_count = m_render_target_desc.size();
    result.subpasses = m_pass_desc.data();
    result.subpass_count = m_pass_desc.size();
    return result;
}

render_target_set_desc render_target_set_info::convert() noexcept
{
    render_target_set_desc result;
    result.render_targets = render_targets.data();
    result.render_target_count = render_targets.size();
    result.width = width;
    result.height = height;
    result.render_pass = render_pass;
    return result;
}
} // namespace ash::graphics