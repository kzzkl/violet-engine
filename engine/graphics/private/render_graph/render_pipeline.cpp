#include "graphics/render_graph/render_pipeline.hpp"
#include "core/context/engine.hpp"
#include "graphics/graphics_module.hpp"

namespace violet
{
render_pipeline::render_pipeline(std::string_view name, std::size_t index)
    : render_node(name, index),
      m_blend{.enable = false},
      m_samples(1),
      m_primitive_topology(PRIMITIVE_TOPOLOGY_TRIANGLE_LIST),
      m_render_pass(nullptr),
      m_subpass(0)
{
    m_blend.enable = false;
    m_samples = 1;
}

render_pipeline::~render_pipeline()
{
}

void render_pipeline::set_shader(std::string_view vertex, std::string_view pixel)
{
    m_vertex_shader = vertex;
    m_pixel_shader = pixel;
}

void render_pipeline::set_vertex_layout(const std::vector<vertex_attribute>& vertex_layout)
{
    m_vertex_layout = vertex_layout;
}

void render_pipeline::set_parameter_layout(
    const std::vector<pipeline_parameter_desc>& parameter_layout)
{
    m_parameter_layout = parameter_layout;
}

void render_pipeline::set_blend(const blend_desc& blend) noexcept
{
    m_blend = blend;
}

void render_pipeline::set_depth_stencil(const depth_stencil_desc& depth_stencil) noexcept
{
    m_depth_stencil = depth_stencil;
}

void render_pipeline::set_rasterizer(const rasterizer_desc& rasterizer) noexcept
{
    m_rasterizer = rasterizer;
}

void render_pipeline::set_samples(std::size_t samples) noexcept
{
    m_samples = samples;
}

void render_pipeline::set_primitive_topology(primitive_topology primitive_topology) noexcept
{
    m_primitive_topology = primitive_topology;
}

void render_pipeline::set_render_pass(render_pass* render_pass, std::size_t subpass) noexcept
{
    m_render_pass = render_pass;
    m_subpass = subpass;
}

bool render_pipeline::compile()
{
    render_pipeline_desc desc = {};
    desc.vertex_shader = m_vertex_shader.c_str();
    desc.pixel_shader = m_pixel_shader.c_str();
    desc.vertex_attributes = m_vertex_layout.data();
    desc.vertex_attribute_count = m_vertex_layout.size();
    desc.parameters = m_parameter_layout.data();
    desc.parameter_count = m_parameter_layout.size();
    desc.blend = m_blend;
    desc.depth_stencil = m_depth_stencil;
    desc.rasterizer = m_rasterizer;
    desc.samples = m_samples;
    desc.primitive_topology = m_primitive_topology;
    desc.render_pass = m_render_pass->get_interface();
    desc.render_subpass_index = m_subpass;

    rhi_context* rhi = engine::get_module<graphics_module>().get_rhi();
    m_interface.reset(rhi->make_render_pipeline(desc));

    return m_interface != nullptr;
}
} // namespace violet