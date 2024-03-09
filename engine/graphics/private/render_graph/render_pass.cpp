#include "graphics/render_graph/render_pass.hpp"
#include "common/hash.hpp"
#include <cassert>

namespace violet
{
render_pass::render_pass(renderer* renderer, setup_context& context)
    : pass(renderer, context),
      m_interface(nullptr),
      m_renderer(renderer)
{
}

render_pass::~render_pass()
{
}

std::size_t render_pass::add_subpass(const std::vector<render_subpass_reference>& references)
{
    rhi_render_subpass_desc desc = {};
    for (auto& reference : references)
    {
        auto& reference_desc = desc.references[desc.reference_count];
        reference_desc.type = reference.type;
        reference_desc.layout = reference.layout;

        auto iter =
            std::find(m_framebuffer_slots.begin(), m_framebuffer_slots.end(), reference.slot);
        if (iter != m_framebuffer_slots.end())
        {
            reference_desc.index = iter - m_framebuffer_slots.begin();
        }
        else
        {
            m_framebuffer_slots.push_back(reference.slot);
            reference_desc.index = m_framebuffer_slots.size() - 1;
        }

        reference_desc.resolve_index = 0;

        ++desc.reference_count;
    }
    m_subpasses.push_back(desc);

    return m_subpasses.size() - 1;
}

render_pipeline* render_pass::add_pipeline(std::size_t subpass_index)
{
    std::size_t color_attachment_count = 0;

    for (std::size_t i = 0; i < m_subpasses[subpass_index].reference_count; ++i)
    {
        if (m_subpasses[subpass_index].references[i].type == RHI_ATTACHMENT_REFERENCE_TYPE_COLOR)
            ++color_attachment_count;
    }
    m_pipelines.push_back(
        std::make_unique<render_pipeline>(this, subpass_index, color_attachment_count));
    return m_pipelines.back().get();
}

void render_pass::add_dependency(
    std::size_t src_subpass,
    rhi_pipeline_stage_flags src_stage,
    rhi_access_flags src_access,
    std::size_t dst_subpass,
    rhi_pipeline_stage_flags dst_stage,
    rhi_access_flags dst_access)
{
    rhi_render_subpass_dependency_desc dependency = {};
    dependency.src = src_subpass;
    dependency.src_stage = src_stage;
    dependency.src_access = src_access;
    dependency.dst = dst_subpass;
    dependency.dst_stage = dst_stage;
    dependency.dst_access = dst_access;
    m_dependencies.push_back(dependency);
}

bool render_pass::compile(compile_context& context)
{
    assert(!m_subpasses.empty());

    rhi_render_pass_desc desc = {};

    for (pass_slot* slot : m_framebuffer_slots)
    {
        auto& attachment = desc.attachments[desc.attachment_count];
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
        else if (slot->get_input_layout() == RHI_IMAGE_LAYOUT_UNDEFINED)
        {
            attachment.load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencil_load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        else
        {
            attachment.load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
            attachment.stencil_load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
        }

        attachment.store_op = RHI_ATTACHMENT_STORE_OP_STORE;
        attachment.stencil_store_op = RHI_ATTACHMENT_STORE_OP_STORE;

        ++desc.attachment_count;
    }

    for (std::size_t i = 0; i < m_subpasses.size(); ++i)
    {
        desc.subpasses[i] = m_subpasses[i];

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
    for (pass_slot* slot : m_framebuffer_slots)
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
    for (pass_slot* slot : m_framebuffer_slots)
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
    return m_framebuffer_slots[0]->get_image()->get_extent();
}
} // namespace violet