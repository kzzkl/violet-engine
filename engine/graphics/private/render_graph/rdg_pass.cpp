#include "graphics/render_graph/rdg_pass.hpp"
#include <algorithm>

namespace violet
{
rdg_pass::rdg_pass() : m_index(-1), m_material_parameter_index(-1)
{
    rdg_pass_reference* reference = add_reference();
    reference->name = "self";
    reference->access = RDG_PASS_ACCESS_FLAG_READ | RDG_PASS_ACCESS_FLAG_WRITE;
    reference->type = RDG_PASS_REFERENCE_TYPE_NONE;
}

rdg_pass::~rdg_pass()
{
}

rdg_pass_reference* rdg_pass::add_texture(
    std::string_view name,
    rdg_pass_access_flags access,
    rhi_texture_layout layout)
{
    rdg_pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = access;
    reference->type = RDG_PASS_REFERENCE_TYPE_TEXTURE;
    reference->texture.layout = layout;
    reference->texture.next_layout = layout;
    return reference;
}

rdg_pass_reference* rdg_pass::add_buffer(std::string_view name, rdg_pass_access_flags access)
{
    rdg_pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = access;
    reference->type = RDG_PASS_REFERENCE_TYPE_BUFFER;
    return reference;
}

rdg_pass_reference* rdg_pass::get_reference(std::string_view name)
{
    for (auto& reference : m_references)
    {
        if (reference->name == name)
            return reference.get();
    }

    throw std::exception("pass does not have this reference.");
}

std::vector<rdg_pass_reference*> rdg_pass::get_references(rdg_pass_access_flags access) const
{
    std::vector<rdg_pass_reference*> result;
    for (auto& reference : m_references)
    {
        if (reference->access & access)
            result.push_back(reference.get());
    }

    return result;
}

std::vector<rdg_pass_reference*> rdg_pass::get_references(rdg_pass_reference_type type) const
{
    std::vector<rdg_pass_reference*> result;
    for (auto& reference : m_references)
    {
        if (reference->type == type)
            result.push_back(reference.get());
    }

    return result;
}

rdg_pass_reference* rdg_pass::add_reference()
{
    m_references.push_back(std::make_unique<rdg_pass_reference>());
    return m_references.back().get();
}

rdg_render_pass::rdg_render_pass()
{
    m_desc = std::make_unique<rhi_render_pipeline_desc>();
}

rdg_pass_reference* rdg_render_pass::add_input(std::string_view name, rhi_texture_layout layout)
{
    rdg_pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = RDG_PASS_ACCESS_FLAG_READ;
    reference->type = RDG_PASS_REFERENCE_TYPE_ATTACHMENT;
    reference->attachment.layout = layout;
    reference->attachment.next_layout = layout;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_INPUT;

    return reference;
}

rdg_pass_reference* rdg_render_pass::add_color(
    std::string_view name,
    rhi_texture_layout layout,
    const rhi_attachment_blend_desc& blend)
{
    rdg_pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = RDG_PASS_ACCESS_FLAG_WRITE;
    reference->type = RDG_PASS_REFERENCE_TYPE_ATTACHMENT;
    reference->attachment.layout = layout;
    reference->attachment.next_layout = layout;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_COLOR;

    m_desc->blend.attachments[m_desc->blend.attachment_count] = blend;
    ++m_desc->blend.attachment_count;

    return reference;
}

rdg_pass_reference* rdg_render_pass::add_depth_stencil(
    std::string_view name,
    rhi_texture_layout layout)
{
    rdg_pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = RDG_PASS_ACCESS_FLAG_READ | RDG_PASS_ACCESS_FLAG_WRITE;
    reference->type = RDG_PASS_REFERENCE_TYPE_ATTACHMENT;
    reference->attachment.layout = layout;
    reference->attachment.next_layout = layout;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL;

    return reference;
}

void rdg_render_pass::set_primitive_topology(rhi_primitive_topology topology) noexcept
{
    m_desc->primitive_topology = topology;
}

void rdg_render_pass::set_cull_mode(rhi_cull_mode mode) noexcept
{
    m_desc->rasterizer.cull_mode = mode;
}

void rdg_render_pass::set_render_pass(
    rhi_render_pass* render_pass,
    std::uint32_t subpass_index) noexcept
{
    m_desc->render_pass = render_pass;
    m_desc->render_subpass_index = subpass_index;
}

void rdg_render_pass::compile(render_device* device)
{
    rhi_ptr<rhi_shader> vertex_shader = device->create_shader(m_vertex_shader.c_str());
    rhi_ptr<rhi_shader> fragment_shader = device->create_shader(m_fragment_shader.c_str());

    m_desc->vertex_shader = vertex_shader.get();
    m_desc->fragment_shader = fragment_shader.get();

    std::vector<rhi_input_desc> inputs;
    for (auto& [name, format] : get_input_layout())
        inputs.push_back({.name = name.c_str(), .format = format});
    m_desc->inputs = inputs.data();
    m_desc->input_count = inputs.size();

    std::vector<rhi_parameter_desc> parameters = get_parameter_layout();
    m_desc->parameters = parameters.data();
    m_desc->parameter_count = parameters.size();

    m_pipeline = device->create_render_pipeline(*m_desc);
    m_desc = nullptr;
}

rdg_compute_pass::rdg_compute_pass()
{
    m_desc = std::make_unique<rhi_compute_pipeline_desc>();
}

void rdg_compute_pass::compile(render_device* device)
{
    rhi_ptr<rhi_shader> compute_shader = device->create_shader(m_compute_shader.c_str());
    m_desc->compute_shader = compute_shader.get();

    std::vector<rhi_parameter_desc> parameters = get_parameter_layout();
    m_desc->parameters = parameters.data();
    m_desc->parameter_count = parameters.size();

    m_pipeline = device->create_compute_pipeline(*m_desc);
    m_desc = nullptr;
}
} // namespace violet