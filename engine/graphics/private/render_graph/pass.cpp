#include "graphics/render_graph/pass.hpp"

namespace violet
{
pass::pass() : m_flags(PASS_FLAG_NONE)
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

    m_blend.push_back(blend);

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

void render_pass::compile(renderer* renderer)
{
    rhi_render_pipeline_desc desc = {};
    desc.vertex_shader = m_vertex_shader.c_str();
    desc.fragment_shader = m_fragment_shader.c_str();

    for (std::size_t i = 0; i < m_blend.size(); ++i)
        desc.blend.attachments[i] = m_blend[i];
    desc.blend.attachment_count = m_blend.size();

    desc.primitive_topology = m_topology;

    desc.render_pass = m_render_pass;
    desc.render_subpass_index = m_subpass_index;

    m_pipeline = renderer->create_render_pipeline(desc);
}

mesh_pass::mesh_pass()
{
}
} // namespace violet