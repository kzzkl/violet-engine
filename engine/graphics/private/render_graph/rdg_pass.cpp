#include "graphics/render_graph/rdg_pass.hpp"
#include <algorithm>
#include <cassert>

namespace violet
{
rdg_pass::rdg_pass()
{
}

rdg_pass::~rdg_pass()
{
}

rdg_reference* rdg_pass::add_texture(
    rdg_texture* texture,
    rhi_access_flags access,
    rhi_texture_layout layout)
{
    rdg_reference* reference = add_reference(texture);
    reference->access = access;
    reference->type = RDG_REFERENCE_TYPE_TEXTURE;
    reference->texture.layout = layout;
    return reference;
}

rdg_reference* rdg_pass::add_buffer(rdg_buffer* buffer, rhi_access_flags access)
{
    rdg_reference* reference = add_reference(buffer);
    reference->access = access;
    reference->type = RDG_REFERENCE_TYPE_BUFFER;
    return reference;
}

std::vector<rdg_reference*> rdg_pass::get_references(rhi_access_flags access) const
{
    std::vector<rdg_reference*> result;
    for (auto& reference : m_references)
    {
        if (reference->access & access)
            result.push_back(reference.get());
    }

    return result;
}

std::vector<rdg_reference*> rdg_pass::get_references(rdg_reference_type type) const
{
    std::vector<rdg_reference*> result;
    for (auto& reference : m_references)
    {
        if (reference->type == type)
            result.push_back(reference.get());
    }

    return result;
}

std::vector<rdg_reference*> rdg_pass::get_references() const
{
    std::vector<rdg_reference*> result;
    for (auto& reference : m_references)
        result.push_back(reference.get());

    return result;
}

rdg_reference* rdg_pass::add_reference(rdg_resource* resource)
{
    m_references.push_back(std::make_unique<rdg_reference>());
    m_references.back()->pass = this;
    m_references.back()->resource = resource;
    return m_references.back().get();
}

rdg_render_pass::rdg_render_pass()
{
}

void rdg_render_pass::add_render_target(
    rdg_texture* render_target,
    rhi_attachment_load_op load_op,
    rhi_attachment_store_op store_op,
    const rhi_attachment_blend& blend)
{
    rdg_reference* reference = add_reference(render_target);
    reference->type = RDG_REFERENCE_TYPE_ATTACHMENT;
    reference->access = RHI_ACCESS_FLAG_SHADER_WRITE;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_COLOR;
    reference->attachment.load_op = load_op;
    reference->attachment.store_op = store_op;

    m_attachments.push_back(reference);
}

void rdg_render_pass::set_depth_stencil(
    rdg_texture* depth_stencil,
    rhi_attachment_load_op load_op,
    rhi_attachment_store_op store_op)
{
    rdg_reference* reference = add_reference(depth_stencil);
    reference->type = RDG_REFERENCE_TYPE_ATTACHMENT;
    reference->access = RHI_ACCESS_FLAG_DEPTH_STENCIL_READ | RHI_ACCESS_FLAG_DEPTH_STENCIL_WRITE;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL;
    reference->attachment.load_op = load_op;
    reference->attachment.store_op = store_op;

    m_attachments.push_back(reference);
}

rdg_compute_pass::rdg_compute_pass()
{
}
} // namespace violet