#include "graphics/render_graph/render_graph.hpp"
#include <cassert>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>

namespace violet
{
render_graph::render_graph(rdg_allocator* allocator) noexcept : m_allocator(allocator)
{
}

render_graph::~render_graph()
{
}

void render_graph::compile()
{
    dead_stripping();

    for (std::size_t i = 0; i < m_resources.size(); ++i)
        m_resources[i]->m_index = i;

    for (std::size_t i = 0; i < m_passes.size(); ++i)
    {
        for (auto& reference : m_passes[i]->get_references())
            reference->resource->add_reference(reference);

        m_passes[i]->m_index = i;
    }

    merge_pass();
}

void render_graph::execute(rhi_command* command)
{
    rdg_command cmd(command, m_allocator);

    auto iter = m_render_passes.begin();
    for (auto& pass : m_passes)
    {
        if (iter != m_render_passes.end() && pass.get() == iter->begin_pass)
            cmd.begin_render_pass(iter->render_pass, iter->framebuffer);

        pass->execute(&cmd);

        if (iter != m_render_passes.end() && pass.get() == iter->end_pass)
            cmd.end_render_pass();
    }
}

void render_graph::dead_stripping()
{
}

void render_graph::merge_pass()
{
    std::vector<rdg_pass*> passes;

    auto merge_pass = [&, this]()
    {
        if (passes.empty())
            return;

        render_pass render_pass = {};
        render_pass.begin_pass = passes[0];
        render_pass.end_pass = passes.back();

        rhi_render_pass_desc render_pass_desc = {};
        rhi_framebuffer_desc framebuffer_desc = {};

        auto& attachments = static_cast<rdg_render_pass*>(passes[0])->get_attachments();
        for (auto& attachment : attachments)
        {
            rdg_texture* texture = static_cast<rdg_texture*>(attachment->resource);

            rhi_texture_layout layout =
                attachment->attachment.type == RHI_ATTACHMENT_REFERENCE_TYPE_COLOR
                    ? RHI_TEXTURE_LAYOUT_RENDER_TARGET
                    : RHI_TEXTURE_LAYOUT_DEPTH_STENCIL;

            render_pass_desc.attachments[render_pass_desc.attachment_count++] = {
                .format = texture->get_format(),
                .samples = RHI_SAMPLE_COUNT_1,
                .initial_layout = texture->is_first_pass(passes[0]) ? texture->get_initial_layout()
                                                                    : RHI_TEXTURE_LAYOUT_UNDEFINED,
                .final_layout =
                    texture->is_last_pass(passes.back()) ? texture->get_final_layout() : layout,
                .load_op = attachment->attachment.load_op,
                .store_op = attachment->attachment.store_op,
                .stencil_load_op = attachment->attachment.load_op,
                .stencil_store_op = attachment->attachment.store_op};

            auto& subpass = render_pass_desc.subpasses[0];
            subpass.references[subpass.reference_count] = {
                .type = attachment->attachment.type,
                .layout = layout,
                .index = subpass.reference_count};
            ++subpass.reference_count;

            framebuffer_desc.attachments[framebuffer_desc.attachment_count++] = texture->get_rhi();
        }
        render_pass_desc.subpass_count = 1;
        render_pass.render_pass = m_allocator->get_render_pass(render_pass_desc);

        framebuffer_desc.render_pass = render_pass.render_pass;
        render_pass.framebuffer = m_allocator->get_framebuffer(framebuffer_desc);

        m_render_passes.push_back(render_pass);

        passes.clear();
    };

    auto check_merge = [](rdg_pass* a, rdg_pass* b) -> bool
    {
        auto& a_attachments = static_cast<rdg_render_pass*>(a)->get_attachments();
        auto& b_attachments = static_cast<rdg_render_pass*>(b)->get_attachments();

        if (a_attachments.size() != b_attachments.size())
            return false;

        for (std::size_t i = 0; i < a_attachments.size(); ++i)
        {
            if (a_attachments[i]->resource != b_attachments[i]->resource)
                return false;
            if (a_attachments[i]->attachment.type != b_attachments[i]->attachment.type)
                return false;
            if (b_attachments[i]->attachment.load_op == RHI_ATTACHMENT_LOAD_OP_CLEAR)
                return false;
        }

        return true;
    };

    for (auto& pass : m_passes)
    {
        if (pass->get_type() != RDG_PASS_TYPE_RENDER)
        {
            merge_pass();
            continue;
        }

        if (passes.empty())
        {
            passes.push_back(pass.get());
            continue;
        }

        if (check_merge(passes[0], pass.get()))
        {
            passes.push_back(pass.get());
            continue;
        }
        else
        {
            merge_pass();
            passes.push_back(pass.get());
        }
    }

    merge_pass();
}

void render_graph::build_barriers()
{
}
} // namespace violet