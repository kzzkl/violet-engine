#include "graphics/render_graph/render_pipeline.hpp"
#include "graphics/render_graph/render_pass.hpp"

namespace violet
{
render_pipeline::render_pipeline(
    render_pass* render_pass,
    std::size_t subpass_index,
    std::size_t color_attachment_count)
    : m_desc{},
      m_render_pass(render_pass)
{
    m_desc.blend.attachment_count = color_attachment_count;
    m_desc.rasterizer.cull_mode = RHI_CULL_MODE_BACK;
    m_desc.samples = RHI_SAMPLE_COUNT_1;
    m_desc.primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    m_desc.render_subpass_index = subpass_index;
}

render_pipeline::~render_pipeline()
{
}

void render_pipeline::set_shader(std::string_view vertex, std::string_view fragment)
{
    m_vertex_shader = vertex;
    m_fragment_shader = fragment;

    m_desc.vertex_shader = m_vertex_shader.c_str();
    m_desc.fragment_shader = m_fragment_shader.c_str();
}

void render_pipeline::set_vertex_attributes(const vertex_attributes& vertex_attributes)
{
    m_vertex_attributes = vertex_attributes;
}

const render_pipeline::vertex_attributes& render_pipeline::get_vertex_attributes() const noexcept
{
    return m_vertex_attributes;
}

void render_pipeline::set_parameter_layouts(
    const std::vector<rhi_parameter_layout*>& parameter_layouts)
{
    m_parameter_layouts = parameter_layouts;
}

void render_pipeline::set_blend(
    std::size_t attachment_index,
    const rhi_attachment_blend_desc& attachment_blend) noexcept
{
    m_desc.blend.attachments[attachment_index] = attachment_blend;
}

void render_pipeline::set_depth_stencil(const rhi_depth_stencil_desc& depth_stencil) noexcept
{
    m_desc.depth_stencil = depth_stencil;
}

void render_pipeline::set_cull_mode(rhi_cull_mode cull_mode) noexcept
{
    m_desc.rasterizer.cull_mode = cull_mode;
}

void render_pipeline::set_samples(rhi_sample_count samples) noexcept
{
    m_desc.samples = samples;
}

void render_pipeline::set_primitive_topology(rhi_primitive_topology primitive_topology) noexcept
{
    m_desc.primitive_topology = primitive_topology;
}

bool render_pipeline::compile(renderer* renderer)
{
    std::vector<rhi_vertex_attribute> vertex_attributes;
    for (auto& attribute : m_vertex_attributes)
        vertex_attributes.push_back({attribute.first.c_str(), attribute.second});

    m_desc.vertex_attributes = vertex_attributes.data();
    m_desc.vertex_attribute_count = vertex_attributes.size();
    m_desc.parameters = m_parameter_layouts.data();
    m_desc.parameter_count = m_parameter_layouts.size();
    m_desc.render_pass = m_render_pass->get_interface();

    m_interface = renderer->create_render_pipeline(m_desc);

    return m_interface != nullptr;
}

void render_pipeline::add_mesh(const render_mesh& mesh)
{
    m_meshes.push_back(mesh);
}

const std::vector<render_mesh>& render_pipeline::get_meshes() const noexcept
{
    return m_meshes;
}

void render_pipeline::clear_mesh()
{
    m_meshes.clear();
}
} // namespace violet