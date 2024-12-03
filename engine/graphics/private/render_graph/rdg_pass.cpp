#include "graphics/render_graph/rdg_pass.hpp"
#include <cassert>

namespace violet
{
rdg_pass::rdg_pass() {}

rdg_pass::~rdg_pass() {}

rdg_reference* rdg_pass::add_texture(
    rdg_texture* texture,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access,
    rhi_texture_layout layout)
{
    rdg_reference* reference = add_reference(texture);
    reference->stages = stages;
    reference->access = access;
    reference->type = RDG_REFERENCE_TEXTURE;
    reference->texture.layout = layout;
    return reference;
}

rdg_reference* rdg_pass::add_buffer(
    rdg_buffer* buffer,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access)
{
    rdg_reference* reference = add_reference(buffer);
    reference->stages = stages;
    reference->access = access;
    reference->type = RDG_REFERENCE_BUFFER;
    return reference;
}

std::vector<rdg_reference*> rdg_pass::get_references(rhi_access_flags access) const
{
    std::vector<rdg_reference*> result;
    for (const auto& reference : m_references)
    {
        if (reference->access & access)
        {
            result.push_back(reference.get());
        }
    }

    return result;
}

std::vector<rdg_reference*> rdg_pass::get_references(rdg_reference_type type) const
{
    std::vector<rdg_reference*> result;
    for (const auto& reference : m_references)
    {
        if (reference->type == type)
        {
            result.push_back(reference.get());
        }
    }

    return result;
}

rdg_reference* rdg_pass::add_reference(rdg_resource* resource)
{
    auto reference = std::make_unique<rdg_reference>();
    reference->pass = this;
    reference->resource = resource;
    m_references.push_back(std::move(reference));
    return m_references.back().get();
}

void rdg_render_pass::add_render_target(
    rdg_texture* texture,
    rhi_attachment_load_op load_op,
    rhi_attachment_store_op store_op)
{
    rdg_reference* reference = add_reference(texture);
    reference->type = RDG_REFERENCE_ATTACHMENT;
    reference->stages = RHI_PIPELINE_STAGE_COLOR_OUTPUT;
    reference->access = RHI_ACCESS_COLOR_WRITE |
                        (load_op == RHI_ATTACHMENT_LOAD_OP_LOAD ? RHI_ACCESS_COLOR_READ : 0);
    reference->attachment.layout = RHI_TEXTURE_LAYOUT_RENDER_TARGET;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_COLOR;
    reference->attachment.load_op = load_op;
    reference->attachment.store_op = store_op;

    m_attachments.push_back(reference);
}

void rdg_render_pass::set_depth_stencil(
    rdg_texture* texture,
    rhi_attachment_load_op load_op,
    rhi_attachment_store_op store_op)
{
    rdg_reference* reference = add_reference(texture);
    reference->type = RDG_REFERENCE_ATTACHMENT;
    reference->stages =
        RHI_PIPELINE_STAGE_EARLY_DEPTH_STENCIL | RHI_PIPELINE_STAGE_LATE_DEPTH_STENCIL;
    reference->access = RHI_ACCESS_DEPTH_STENCIL_READ | RHI_ACCESS_DEPTH_STENCIL_WRITE;
    reference->attachment.layout = RHI_TEXTURE_LAYOUT_DEPTH_STENCIL;
    reference->attachment.type = RHI_ATTACHMENT_REFERENCE_DEPTH_STENCIL;
    reference->attachment.load_op = load_op;
    reference->attachment.store_op = store_op;

    m_attachments.push_back(reference);
}

rdg_compute_pass::rdg_compute_pass() {}
} // namespace violet