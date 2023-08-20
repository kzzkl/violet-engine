#include "graphics/render_graph/render_pipeline.hpp"
#include "graphics/render_graph/render_pass.hpp"

namespace violet
{
render_pipeline::render_pipeline(std::string_view name, rhi_context* rhi)
    : render_node(name, rhi),
      m_blend{.enable = false},
      m_samples(RHI_SAMPLE_COUNT_1),
      m_primitive_topology(RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
{
}

render_pipeline::~render_pipeline()
{
    if (m_interface != nullptr)
        get_rhi()->destroy_render_pipeline(m_interface);
}

void render_pipeline::set_shader(std::string_view vertex, std::string_view pixel)
{
    m_vertex_shader = vertex;
    m_pixel_shader = pixel;
}

void render_pipeline::set_vertex_layout(const vertex_layout& vertex_layout)
{
    m_vertex_layout = vertex_layout;
}

const render_pipeline::vertex_layout& render_pipeline::get_vertex_layout() const noexcept
{
    return m_vertex_layout;
}

void render_pipeline::set_parameter_layout(
    const std::vector<rhi_pipeline_parameter_desc>& parameter_layout)
{
    m_parameter_layout = parameter_layout;
}

void render_pipeline::set_blend(const rhi_blend_desc& blend) noexcept
{
    m_blend = blend;
}

void render_pipeline::set_depth_stencil(const rhi_depth_stencil_desc& depth_stencil) noexcept
{
    m_depth_stencil = depth_stencil;
}

void render_pipeline::set_rasterizer(const rhi_rasterizer_desc& rasterizer) noexcept
{
    m_rasterizer = rasterizer;
}

void render_pipeline::set_samples(rhi_sample_count samples) noexcept
{
    m_samples = samples;
}

void render_pipeline::set_primitive_topology(rhi_primitive_topology primitive_topology) noexcept
{
    m_primitive_topology = primitive_topology;
}

bool render_pipeline::compile(rhi_render_pass* render_pass, std::size_t subpass_index)
{
    std::vector<rhi_vertex_attribute> vertex_attributes;
    for (auto& attribute : m_vertex_layout)
        vertex_attributes.push_back({attribute.first.c_str(), attribute.second});

    rhi_render_pipeline_desc desc = {};
    desc.vertex_shader = m_vertex_shader.c_str();
    desc.pixel_shader = m_pixel_shader.c_str();
    desc.vertex_attributes = vertex_attributes.data();
    desc.vertex_attribute_count = vertex_attributes.size();
    desc.parameters = m_parameter_layout.data();
    desc.parameter_count = m_parameter_layout.size();
    desc.blend = m_blend;
    desc.depth_stencil = m_depth_stencil;
    desc.rasterizer = m_rasterizer;
    desc.samples = m_samples;
    desc.primitive_topology = m_primitive_topology;
    desc.render_pass = render_pass;
    desc.render_subpass_index = subpass_index;

    m_interface = get_rhi()->make_render_pipeline(desc);

    return m_interface != nullptr;
}

void render_pipeline::execute(rhi_render_command* command)
{
    command->set_pipeline(m_interface);
    command->draw(0, 3);
}
} // namespace violet