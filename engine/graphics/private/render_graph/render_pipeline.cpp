#include "graphics/render_graph/render_pipeline.hpp"
#include "graphics/render_graph/render_pass.hpp"

namespace violet
{
render_pipeline::render_pipeline(std::string_view name, rhi_context* rhi) : render_node(name, rhi)
{
    m_desc.blend.enable = false;
    m_desc.samples = RHI_SAMPLE_COUNT_1;
    m_desc.primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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

    m_desc.vertex_shader = m_vertex_shader.c_str();
    m_desc.pixel_shader = m_pixel_shader.c_str();
}

void render_pipeline::set_vertex_layout(const vertex_layout& vertex_layout)
{
    m_vertex_layout = vertex_layout;
}

const render_pipeline::vertex_layout& render_pipeline::get_vertex_layout() const noexcept
{
    return m_vertex_layout;
}

void render_pipeline::set_parameter_layout(const parameter_layout& parameter_layout)
{
    m_parameter_layout = parameter_layout;
}

rhi_pipeline_parameter_layout* render_pipeline::get_parameter_layout(
    render_pipeline_parameter_type type) const noexcept
{
    for (auto& parameter : m_parameter_layout)
    {
        if (parameter.second == type)
            return parameter.first;
    }
    return nullptr;
}

void render_pipeline::set_blend(const rhi_blend_desc& blend) noexcept
{
    m_desc.blend = blend;
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

bool render_pipeline::compile(rhi_render_pass* render_pass, std::size_t subpass_index)
{
    std::vector<rhi_vertex_attribute> vertex_attributes;
    for (auto& attribute : m_vertex_layout)
        vertex_attributes.push_back({attribute.first.c_str(), attribute.second});

    m_desc.vertex_attributes = vertex_attributes.data();
    m_desc.vertex_attribute_count = vertex_attributes.size();
    m_desc.render_pass = render_pass;
    m_desc.render_subpass_index = subpass_index;

    std::vector<rhi_pipeline_parameter_layout*> parameter_layout;
    for (auto& parameter : m_parameter_layout)
        parameter_layout.push_back(parameter.first);

    m_desc.parameters = parameter_layout.data();
    m_desc.parameter_count = parameter_layout.size();

    m_interface = get_rhi()->create_render_pipeline(m_desc);

    return m_interface != nullptr;
}

void render_pipeline::execute(rhi_render_command* command)
{
    command->set_pipeline(m_interface);
    render(command, m_context);
    m_context.meshes.clear();
}

void render_pipeline::add_mesh(const render_mesh& mesh)
{
    m_context.meshes.push_back(mesh);
}

void render_pipeline::set_camera_parameter(rhi_pipeline_parameter* parameter) noexcept
{
    m_context.camera_parameter = parameter;
}

void render_pipeline::render(rhi_render_command* command, render_context& context)
{
    for (render_mesh& mesh : m_context.meshes)
    {
        command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
        command->set_index_buffer(mesh.index_buffer);
        command->set_parameter(0, mesh.node);
        command->set_parameter(1, mesh.material);
        command->set_parameter(2, context.camera_parameter);
        command->draw_indexed(0, 12, 0);
    }
}
} // namespace violet