#include "render_graph/rdg_pass_batch.hpp"
#include "common/hash.hpp"
#include <algorithm>
#include <cassert>

namespace violet
{
rdg_render_pass_batch::rdg_render_pass_batch(
    const std::vector<rdg_pass*> passes,
    render_device* device)
    : m_device(device),
      m_execute_count(0)
{
    std::vector<rdg_render_pass*> render_passes;
    for (rdg_pass* pass : passes)
    {
        assert(pass->get_type() == RDG_PASS_TYPE_RENDER);
        render_passes.push_back(static_cast<rdg_render_pass*>(pass));
    }

    std::vector<std::pair<std::size_t, std::size_t>> subpass_ranges;
    subpass_ranges.push_back({0, 1});

    for (std::size_t i = 1; i < render_passes.size(); ++i)
    {
        rdg_render_pass* prev_pass = render_passes[i - 1];
        rdg_render_pass* next_pass = render_passes[i];

        auto prev_references = prev_pass->get_references(RDG_PASS_REFERENCE_TYPE_ATTACHMENT);
        auto next_references = next_pass->get_references(RDG_PASS_REFERENCE_TYPE_ATTACHMENT);

        bool same = true;
        if (prev_references.size() != next_references.size())
        {
            same = false;
        }
        else
        {
            for (std::size_t j = 0; j < prev_references.size(); ++j)
            {
                auto& prev_reference = prev_references[j];
                auto& next_reference = next_references[j];

                if (prev_reference->resource != next_reference->resource ||
                    prev_reference->attachment.layout != next_reference->attachment.layout)
                {
                    same = false;
                    break;
                }
            }
        }

        if (same)
        {
            ++subpass_ranges.back().second;
        }
        else
        {
            subpass_ranges.back().second = i;
            subpass_ranges.push_back({i, i + 1});
        }
    }

    for (rdg_render_pass* pass : render_passes)
    {
        for (rdg_pass_reference* reference :
             pass->get_references(RDG_PASS_REFERENCE_TYPE_ATTACHMENT))
            m_attachments.push_back(reference->resource);
    }
    std::sort(m_attachments.begin(), m_attachments.end());
    m_attachments.erase(
        std::unique(m_attachments.begin(), m_attachments.end()),
        m_attachments.end());

    rhi_render_pass_desc desc = {};

    std::vector<rhi_attachment_desc> attachments(m_attachments.size());
    std::vector<std::uint8_t> attachment_visited(m_attachments.size());
    for (rdg_render_pass* pass : render_passes)
    {
        for (rdg_pass_reference* reference :
             pass->get_references(RDG_PASS_REFERENCE_TYPE_ATTACHMENT))
        {
            std::size_t index =
                std::find(m_attachments.begin(), m_attachments.end(), reference->resource) -
                m_attachments.begin();

            if (attachment_visited[index] == 0)
            {
                attachments[index].format =
                    static_cast<rdg_texture*>(reference->resource)->get_format();
                attachments[index].samples =
                    static_cast<rdg_texture*>(reference->resource)->get_samples();
                attachments[index].initial_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;

                attachments[index].load_op = reference->attachment.load_op;
                attachments[index].stencil_load_op = reference->attachment.load_op;

                attachment_visited[index] = 1;
            }

            attachments[index].store_op = reference->attachment.store_op;
            attachments[index].stencil_store_op = reference->attachment.store_op;
            attachments[index].final_layout = reference->attachment.next_layout;
        }
    }
    desc.attachments = attachments.data();
    desc.attachment_count = attachments.size();

    std::vector<rhi_render_subpass_desc> subpasses;
    for (auto& [begin, end] : subpass_ranges)
    {
        rhi_render_subpass_desc subpass = {};

        rdg_render_pass* first_pass = render_passes[begin];
        for (rdg_pass_reference* reference :
             first_pass->get_references(RDG_PASS_REFERENCE_TYPE_ATTACHMENT))
        {
            auto& subpass_reference = subpass.references[subpass.reference_count];
            subpass_reference.type = reference->attachment.type;
            subpass_reference.layout = reference->attachment.layout;
            subpass_reference.index =
                std::find(m_attachments.begin(), m_attachments.end(), reference->resource) -
                m_attachments.begin();
            subpass_reference.resolve_index = 0;

            ++subpass.reference_count;
        }
        subpasses.push_back(subpass);
    }
    desc.subpasses = subpasses.data();
    desc.subpass_count = subpasses.size();

    m_render_pass = device->create_render_pass(desc);

    for (auto& [begin, end] : subpass_ranges)
    {
        m_passes.push_back({});
        for (std::size_t i = begin; i < end; ++i)
        {
            render_passes[i]->set_render_pass(m_render_pass.get(), m_passes.size() - 1);
            render_passes[i]->compile(device);
            m_passes.back().push_back(render_passes[i]);
        }
    }
}

void rdg_render_pass_batch::execute(rhi_command* command, rdg_context* context)
{
    std::size_t hash = 0;
    for (auto attachment : m_attachments)
        hash = hash_combine(hash, context->get_texture(attachment->get_index())->get_hash());

    rhi_framebuffer* framebuffer = nullptr;

    auto iter = m_framebuffer_cache.find(hash);
    if (iter == m_framebuffer_cache.end())
    {
        std::vector<rhi_texture*> attachments;
        for (std::size_t i = 0; i < m_attachments.size(); ++i)
            attachments.push_back(context->get_texture(m_attachments[i]->get_index()));

        rhi_framebuffer_desc desc = {};
        desc.attachments = attachments.data();
        desc.attachment_count = attachments.size();
        desc.render_pass = m_render_pass.get();

        m_framebuffer_cache[hash].framebuffer = m_device->create_framebuffer(desc);
        m_framebuffer_cache[hash].is_used = true;
        framebuffer = m_framebuffer_cache[hash].framebuffer.get();
    }
    else
    {
        framebuffer = iter->second.framebuffer.get();
        iter->second.is_used = true;
    }

    command->begin(m_render_pass.get(), framebuffer);

    rhi_texture_extent extent = context->get_texture(m_attachments[0]->get_index())->get_extent();

    rhi_viewport viewport = {};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.min_depth = 0.0f;
    viewport.max_depth = 1.0f;
    command->set_viewport(viewport);

    rhi_scissor_rect scissor = {};
    scissor.max_x = extent.width;
    scissor.max_y = extent.height;
    command->set_scissor(&scissor, 1);

    for (std::size_t i = 0; i < m_passes.size(); ++i)
    {
        for (auto& pass : m_passes[i])
            pass->execute(command, context);

        if (i < m_passes.size() - 1)
            command->next();
    }
    command->end();

    ++m_execute_count;
    if (m_execute_count % FRAMEBUFFER_CLEANUP_INTERVAL == 0)
        cleanup_framebuffer();
}

void rdg_render_pass_batch::cleanup_framebuffer()
{
    for (auto iter = m_framebuffer_cache.begin(); iter != m_framebuffer_cache.end();)
    {
        if (!iter->second.is_used)
        {
            iter = m_framebuffer_cache.erase(iter);
        }
        else
        {
            iter->second.is_used = false;
            ++iter;
        }
    }
}

rdg_compute_pass_batch::rdg_compute_pass_batch(
    const std::vector<rdg_pass*> passes,
    render_device* device)
    : m_passes(passes)
{
    for (rdg_pass* pass : m_passes)
        pass->compile(device);
}

void rdg_compute_pass_batch::execute(rhi_command* command, rdg_context* context)
{
    for (rdg_pass* pass : m_passes)
        pass->execute(command, context);
}

rdg_other_pass_batch::rdg_other_pass_batch(
    const std::vector<rdg_pass*> passes,
    render_device* device)
    : m_passes(passes)
{
    for (rdg_pass* pass : m_passes)
        pass->compile(device);
}

void rdg_other_pass_batch::execute(rhi_command* command, rdg_context* context)
{
    for (rdg_pass* pass : m_passes)
        pass->execute(command, context);
}
} // namespace violet