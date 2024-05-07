#include "render_graph/pass_batch.hpp"
#include "common/hash.hpp"
#include <algorithm>
#include <cassert>

namespace violet
{
render_pass_batch::render_pass_batch(const std::vector<pass*> passes, renderer* renderer)
    : m_renderer(renderer),
      m_execute_count(0)
{
    std::vector<render_pass*> render_passes;
    for (pass* pass : passes)
    {
        assert(pass->get_flags() & PASS_FLAG_RENDER);
        render_passes.push_back(static_cast<render_pass*>(pass));
    }

    std::vector<std::pair<std::size_t, std::size_t>> subpass_ranges;
    subpass_ranges.push_back({0, 1});

    for (std::size_t i = 1; i < render_passes.size(); ++i)
    {
        render_pass* prev_pass = render_passes[i - 1];
        render_pass* next_pass = render_passes[i];

        auto prev_references = prev_pass->get_references(PASS_REFERENCE_TYPE_ATTACHMENT);
        auto next_references = next_pass->get_references(PASS_REFERENCE_TYPE_ATTACHMENT);

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

        if (!same)
        {
            subpass_ranges.back().second = i;
            subpass_ranges.push_back({i, i + 1});
        }
    }

    for (render_pass* pass : render_passes)
    {
        for (pass_reference* reference : pass->get_references(PASS_REFERENCE_TYPE_ATTACHMENT))
            m_attachments.push_back(reference->resource);
    }
    std::sort(m_attachments.begin(), m_attachments.end());
    m_attachments.erase(
        std::unique(m_attachments.begin(), m_attachments.end()),
        m_attachments.end());

    rhi_render_pass_desc desc = {};

    std::vector<std::uint8_t> attachment_visited(m_attachments.size());
    for (render_pass* pass : render_passes)
    {
        for (pass_reference* reference : pass->get_references(PASS_REFERENCE_TYPE_ATTACHMENT))
        {
            std::size_t index =
                std::find(m_attachments.begin(), m_attachments.end(), reference->resource) -
                m_attachments.begin();

            if (attachment_visited[index] == 0)
            {
                desc.attachments[index].format =
                    static_cast<texture*>(reference->resource)->get_format();
                desc.attachments[index].samples =
                    static_cast<texture*>(reference->resource)->get_samples();
                desc.attachments[index].initial_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;

                desc.attachments[index].load_op = reference->attachment.load_op;
                desc.attachments[index].stencil_load_op = reference->attachment.load_op;

                attachment_visited[index] = 1;
            }

            desc.attachments[index].store_op = reference->attachment.store_op;
            desc.attachments[index].stencil_store_op = reference->attachment.store_op;
            desc.attachments[index].final_layout = reference->attachment.next_layout;
        }
    }
    desc.attachment_count = m_attachments.size();

    for (auto& [begin, end] : subpass_ranges)
    {
        auto& subpass = desc.subpasses[desc.subpass_count];

        render_pass* first_pass = render_passes[begin];
        for (pass_reference* reference : first_pass->get_references(PASS_REFERENCE_TYPE_ATTACHMENT))
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

        ++desc.subpass_count;
    }

    m_render_pass = renderer->create_render_pass(desc);

    for (auto& [begin, end] : subpass_ranges)
    {
        m_passes.push_back({});
        for (std::size_t i = begin; i < end; ++i)
        {
            render_passes[i]->set_render_pass(m_render_pass.get(), m_passes.size() - 1);
            render_passes[i]->compile(renderer);
            m_passes.back().push_back(render_passes[i]);
        }
    }
}

void render_pass_batch::execute(rhi_render_command* command, render_context* context)
{
    std::hash<rhi_texture*> hasher;

    std::size_t hash = 0;
    for (auto attachment : m_attachments)
        hash = hash_combine(hash, hasher(attachment->get_texture()));

    rhi_framebuffer* framebuffer = nullptr;

    auto iter = m_framebuffer_cache.find(hash);
    if (iter == m_framebuffer_cache.end())
    {
        rhi_framebuffer_desc desc = {};
        for (std::size_t i = 0; i < m_attachments.size(); ++i)
            desc.attachments[i] = m_attachments[i]->get_texture();
        desc.attachment_count = m_attachments.size();
        desc.render_pass = m_render_pass.get();

        m_framebuffer_cache[hash].framebuffer = m_renderer->create_framebuffer(desc);
        m_framebuffer_cache[hash].is_used = true;
        framebuffer = m_framebuffer_cache[hash].framebuffer.get();
    }
    else
    {
        framebuffer = iter->second.framebuffer.get();
        iter->second.is_used = true;
    }

    command->begin(m_render_pass.get(), framebuffer);
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

void render_pass_batch::cleanup_framebuffer()
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
} // namespace violet