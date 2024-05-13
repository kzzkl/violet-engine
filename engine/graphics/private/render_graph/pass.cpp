#include "graphics/render_graph/pass.hpp"
#include <algorithm>

namespace violet
{
pass::pass()
{
}

pass::~pass()
{
}

pass_reference* pass::add_texture(
    std::string_view name,
    pass_access_flags access,
    rhi_texture_layout layout)
{
    pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = access;
    reference->type = PASS_REFERENCE_TYPE_TEXTURE;
    reference->texture.layout = layout;
    reference->texture.next_layout = layout;
    return reference;
}

pass_reference* pass::add_buffer(std::string_view name, pass_access_flags access)
{
    pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = access;
    reference->type = PASS_REFERENCE_TYPE_BUFFER;
    return reference;
}

pass_reference* pass::get_reference(std::string_view name)
{
    for (auto& reference : m_references)
    {
        if (reference->name == name)
            return reference.get();
    }

    throw std::exception("pass does not have this reference.");
}

std::vector<pass_reference*> pass::get_references(pass_access_flags access) const
{
    std::vector<pass_reference*> result;
    for (auto& reference : m_references)
    {
        if (reference->access & access)
            result.push_back(reference.get());
    }

    return result;
}

std::vector<pass_reference*> pass::get_references(pass_reference_type type) const
{
    std::vector<pass_reference*> result;
    for (auto& reference : m_references)
    {
        if (reference->type == type)
            result.push_back(reference.get());
    }

    return result;
}

pass_reference* pass::add_reference()
{
    m_references.push_back(std::make_unique<pass_reference>());
    return m_references.back().get();
}

render_pass::render_pass()
{
    m_desc = std::make_unique<rhi_render_pipeline_desc>();
}

pass_reference* render_pass::add_input(std::string_view name, rhi_texture_layout layout)
{
    pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = PASS_ACCESS_FLAG_READ;
    reference->type = PASS_REFERENCE_TYPE_ATTACHMENT;
    reference->attachment.layout = layout;
    reference->attachment.next_layout = layout;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_INPUT;

    return reference;
}

pass_reference* render_pass::add_color(
    std::string_view name,
    rhi_texture_layout layout,
    const rhi_attachment_blend_desc& blend)
{
    pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = PASS_ACCESS_FLAG_WRITE;
    reference->type = PASS_REFERENCE_TYPE_ATTACHMENT;
    reference->attachment.layout = layout;
    reference->attachment.next_layout = layout;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_COLOR;

    m_desc->blend.attachments[m_desc->blend.attachment_count] = blend;
    ++m_desc->blend.attachment_count;

    return reference;
}

pass_reference* render_pass::add_depth_stencil(std::string_view name, rhi_texture_layout layout)
{
    pass_reference* reference = add_reference();
    reference->name = name;
    reference->access = PASS_ACCESS_FLAG_READ | PASS_ACCESS_FLAG_WRITE;
    reference->type = PASS_REFERENCE_TYPE_ATTACHMENT;
    reference->attachment.layout = layout;
    reference->attachment.next_layout = layout;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL;

    return reference;
}

void render_pass::set_primitive_topology(rhi_primitive_topology topology) noexcept
{
    m_desc->primitive_topology = topology;
}

const std::vector<std::string>& render_pass::get_vertex_attribute_layout() const noexcept
{
    return m_attributes;
}

void render_pass::set_render_pass(
    rhi_render_pass* render_pass,
    std::uint32_t subpass_index) noexcept
{
    m_desc->render_pass = render_pass;
    m_desc->render_subpass_index = subpass_index;
}

void render_pass::compile(renderer* renderer)
{
    rhi_ptr<rhi_shader> vertex_shader = renderer->create_shader(m_vertex_shader.c_str());
    rhi_ptr<rhi_shader> fragment_shader = renderer->create_shader(m_fragment_shader.c_str());

    m_desc->vertex_shader = vertex_shader.get();
    m_desc->fragment_shader = fragment_shader.get();

    std::vector<rhi_parameter_desc> parameters = get_parameter_layout();
    m_desc->parameters = parameters.data();
    m_desc->parameter_count = parameters.size();

    m_pipeline = renderer->create_render_pipeline(*m_desc);
    m_desc = nullptr;

    std::size_t attribute_count = vertex_shader->get_input_count();
    for (std::size_t i = 0; i < attribute_count; ++i)
        m_attributes.push_back(vertex_shader->get_input_name(i));
}

mesh_pass::mesh_pass()
{
}
} // namespace violet