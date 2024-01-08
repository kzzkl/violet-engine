#include "graphics/render_graph/render_pass.hpp"
#include "common/hash.hpp"
#include <cassert>

namespace violet
{
render_attachment::render_attachment(pass_slot* slot, std::size_t index) : m_index(index), m_desc{}
{
    m_desc.format = slot->get_format();
    m_desc.samples = slot->get_samples();
}

render_subpass::render_subpass(render_pass* render_pass, std::size_t index)
    : m_render_pass(render_pass),
      m_index(index),
      m_desc{}
{
}

void render_subpass::add_reference(
    pass_slot* slot,
    rhi_attachment_reference_type type,
    rhi_image_layout layout)
{
    auto& desc = m_desc.references[m_desc.reference_count];
    desc.type = type;
    desc.layout = layout;
    desc.index = slot->get_index();
    desc.resolve_index = 0;

    ++m_desc.reference_count;
}

render_pass::render_pass(renderer* renderer, setup_context& context)
    : pass(renderer, context),
      m_interface(nullptr),
      m_renderer(renderer)
{
}

render_pass::~render_pass()
{
}

render_subpass* render_pass::add_subpass()
{
    m_subpasses.push_back(std::make_unique<render_subpass>(this, m_subpasses.size()));
    return m_subpasses.back().get();
}

render_pipeline* render_pass::add_pipeline(render_subpass* subpass)
{
    m_pipelines.push_back(std::make_unique<render_pipeline>(this, subpass->get_index()));
    return m_pipelines.back().get();
}

void render_pass::add_dependency(
    render_subpass* src,
    rhi_pipeline_stage_flags src_stage,
    rhi_access_flags src_access,
    render_subpass* dst,
    rhi_pipeline_stage_flags dst_stage,
    rhi_access_flags dst_access)
{
    rhi_render_subpass_dependency_desc dependency = {};
    dependency.src = src == nullptr ? RHI_RENDER_SUBPASS_EXTERNAL : src->get_index();
    dependency.src_stage = src_stage;
    dependency.src_access = src_access;
    dependency.dst = dst == nullptr ? RHI_RENDER_SUBPASS_EXTERNAL : dst->get_index();
    dependency.dst_stage = dst_stage;
    dependency.dst_access = dst_access;
    m_dependencies.push_back(dependency);
}

bool render_pass::compile(compile_context& context)
{
    assert(!m_subpasses.empty());

    rhi_render_pass_desc desc = {};

    for (auto& slot : get_slots())
    {
        auto& attachment = desc.attachments[slot->get_index()];
        attachment.format = slot->get_format() == RHI_RESOURCE_FORMAT_UNDEFINED
                                ? context.renderer->get_back_buffer()->get_format()
                                : slot->get_format();
        attachment.samples = slot->get_samples();
        attachment.initial_layout = slot->get_input_layout();
        attachment.final_layout = slot->get_output_layout();

        if (slot->is_clear())
        {
            attachment.load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
            attachment.stencil_load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        }
        else
        {
            attachment.load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
            attachment.stencil_load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
        }

        attachment.store_op = RHI_ATTACHMENT_STORE_OP_STORE;
        attachment.stencil_store_op = RHI_ATTACHMENT_STORE_OP_STORE;
    }
    desc.attachment_count = get_slots().size();

    for (std::size_t i = 0; i < m_subpasses.size(); ++i)
    {
        desc.subpasses[i] = m_subpasses[i]->get_desc();

        for (std::size_t j = 0; j < desc.subpasses[i].reference_count; ++j)
        {
            auto& reference = desc.attachments[desc.subpasses[i].references[j].index];
            if (reference.final_layout == RHI_IMAGE_LAYOUT_UNDEFINED)
                reference.final_layout = desc.subpasses[i].references[j].layout;
        }
    }
    desc.subpass_count = m_subpasses.size();

    for (std::size_t i = 0; i < m_dependencies.size(); ++i)
        desc.dependencies[i] = m_dependencies[i];
    desc.dependency_count = m_dependencies.size();

    m_interface = context.renderer->create_render_pass(desc);

    if (m_interface == nullptr)
        return false;

    for (auto& pipeline : m_pipelines)
    {
        if (!pipeline->compile(context.renderer))
            return false;
    }

    return true;
}

void render_pass::execute(execute_context& context)
{
}

rhi_framebuffer* render_pass::get_framebuffer()
{
    bool cache = true;

    std::size_t hash = 0;
    for (auto& slot : get_slots())
    {
        if (!slot->is_framebuffer_cache())
        {
            cache = false;
            break;
        }

        hash = hash_combine(hash, slot->get_image()->get_hash());
    }

    if (cache)
    {
        auto iter = m_framebuffer_cache.find(hash);
        if (iter != m_framebuffer_cache.end())
            return iter->second.get();
    }

    rhi_framebuffer_desc desc = {};
    desc.render_pass = m_interface.get();
    for (auto& slot : get_slots())
    {
        desc.attachments[desc.attachment_count] = slot->get_image();
        ++desc.attachment_count;
    }

    if (cache)
    {
        m_framebuffer_cache[hash] = m_renderer->create_framebuffer(desc);
        return m_framebuffer_cache[hash].get();
    }
    else
    {
        m_temporary_framebuffer = m_renderer->create_framebuffer(desc);
        return m_temporary_framebuffer.get();
    }
}

rhi_resource_extent render_pass::get_extent() const
{
    return get_slots()[0]->get_image()->get_extent();
}
} // namespace violet