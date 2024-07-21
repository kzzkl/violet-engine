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

rdg_texture* render_graph::add_texture(
    std::string_view name,
    rhi_texture* texture,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
{
    assert(texture != nullptr);

    auto resource = std::make_unique<rdg_texture>(texture, initial_layout, final_layout);
    resource->m_name = name;

    rdg_texture* result = resource.get();
    m_resources.push_back(std::move(resource));
    return result;
}

rdg_texture* render_graph::add_texture(
    std::string_view name,
    const rhi_texture_desc& desc,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
{
    auto resource = std::make_unique<rdg_inter_texture>(desc, initial_layout, final_layout);
    resource->m_name = name;

    rdg_texture* result = resource.get();
    m_resources.push_back(std::move(resource));
    return result;
}

rdg_texture* render_graph::add_texture(
    std::string_view name,
    rhi_texture* texture,
    std::uint32_t level,
    std::uint32_t layer,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
{
    assert(texture != nullptr);

    auto resource =
        std::make_unique<rdg_texture_view>(texture, level, layer, initial_layout, final_layout);
    resource->m_name = name;

    rdg_texture* result = resource.get();
    m_resources.push_back(std::move(resource));
    return result;
}

rdg_texture* render_graph::add_texture(
    std::string_view name,
    rdg_texture* texture,
    std::uint32_t level,
    std::uint32_t layer,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
{
    assert(texture != nullptr);

    auto resource =
        std::make_unique<rdg_texture_view>(texture, level, layer, initial_layout, final_layout);
    resource->m_name = name;

    rdg_texture* result = resource.get();
    m_resources.push_back(std::move(resource));
    return result;
}

rdg_buffer* render_graph::add_buffer(std::string_view name, rhi_buffer* buffer)
{
    assert(buffer != nullptr);

    auto resource = std::make_unique<rdg_buffer>(buffer);
    resource->m_name = name;

    rdg_buffer* result = resource.get();
    m_resources.push_back(std::move(resource));
    return result;
}

void render_graph::compile()
{
    dead_stripping();

    for (std::size_t i = 0; i < m_resources.size(); ++i)
    {
        m_resources[i]->m_index = i;
        if (m_resources[i]->get_type() == RDG_RESOURCE_TEXTURE)
        {
            rdg_texture* texture = static_cast<rdg_texture*>(m_resources[i].get());
            if (texture->is_texture_view())
            {
                rdg_texture_view* texture_view = static_cast<rdg_texture_view*>(texture);
                rhi_texture_view_desc desc = {
                    .texture = texture_view->m_rhi_texture ? texture_view->m_rhi_texture
                                                           : texture_view->m_rdg_texture->m_texture,
                    .level = texture_view->m_level,
                    .level_count = 1,
                    .layer = texture_view->m_layer,
                    .layer_count = 1};
                texture->m_texture = m_allocator->allocate_texture(desc);
            }
            else if (!m_resources[i]->is_external())
            {
                texture->m_texture = m_allocator->allocate_texture(
                    static_cast<rdg_inter_texture*>(texture)->get_desc());
            }
        }
    }

    for (std::size_t i = 0; i < m_passes.size(); ++i)
    {
        for (auto& reference : m_passes[i]->get_references())
            reference->resource->add_reference(reference);

        m_passes[i]->m_index = i;
    }

    merge_pass();
    build_barriers();
}

void render_graph::execute(rhi_command* command)
{
    rdg_command cmd(command, m_allocator);

    auto iter = m_render_passes.begin();
    for (auto& pass : m_passes)
    {
        auto& barriers = m_barriers[pass->get_index()];
        if (!barriers.texture_barriers.empty() || !barriers.buffer_barriers.empty())
        {
            command->set_pipeline_barrier(
                barriers.src_stages,
                barriers.dst_stages,
                barriers.buffer_barriers.data(),
                barriers.buffer_barriers.size(),
                barriers.texture_barriers.data(),
                barriers.texture_barriers.size());
        }

        if (iter != m_render_passes.end() && pass.get() == iter->begin_pass)
        {
            cmd.m_render_pass = iter->render_pass;
            cmd.m_subpass_index = 0;
            command->begin_render_pass(iter->render_pass, iter->framebuffer);
        }

        pass->execute(&cmd);

        if (iter != m_render_passes.end() && pass.get() == iter->end_pass)
        {
            command->end_render_pass();
            cmd.m_render_pass = nullptr;
            cmd.m_subpass_index = 0;
            ++iter;
        }
    }

    auto& last_barriers = m_barriers.back();
    if (!last_barriers.texture_barriers.empty() || !last_barriers.buffer_barriers.empty())
    {
        command->set_pipeline_barrier(
            last_barriers.src_stages,
            last_barriers.dst_stages,
            last_barriers.buffer_barriers.data(),
            last_barriers.buffer_barriers.size(),
            last_barriers.texture_barriers.data(),
            last_barriers.texture_barriers.size());
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
        for (std::size_t i = 0; i < attachments.size(); ++i)
        {
            rdg_reference* attachment = attachments[i];
            if (attachment->attachment.type == RHI_ATTACHMENT_REFERENCE_COLOR)
                ++render_pass.render_target_count;

            rdg_texture* texture = static_cast<rdg_texture*>(attachment->resource);

            auto& references = texture->get_references();
            rhi_texture_layout initial_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;
            rhi_texture_layout final_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;
            if (references[0]->pass == passes[0])
            {
                initial_layout = texture->get_initial_layout();
            }
            else
            {
                initial_layout = references[attachment->index - 1]->get_texture_layout();
            }

            if (references.back()->pass == passes.back())
            {
                final_layout = texture->get_final_layout();
            }
            else
            {
                final_layout = references[attachment->index + 1]->get_texture_layout();
            }

            render_pass_desc.attachments[render_pass_desc.attachment_count++] = {
                .format = texture->get_format(),
                .samples = RHI_SAMPLE_COUNT_1,
                .initial_layout = initial_layout,
                .final_layout = final_layout,
                .load_op = attachment->attachment.load_op,
                .store_op = attachment->attachment.store_op,
                .stencil_load_op = attachment->attachment.load_op,
                .stencil_store_op = attachment->attachment.store_op};

            auto& subpass = render_pass_desc.subpasses[0];
            subpass.references[subpass.reference_count] = {
                .type = attachment->attachment.type,
                .layout = attachment->get_texture_layout(),
                .index = subpass.reference_count};
            ++subpass.reference_count;

            framebuffer_desc.attachments[i] = texture->get_rhi();
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
        if (pass->get_type() != RDG_PASS_RENDER)
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
    m_barriers.clear();
    m_barriers.resize(m_passes.size() + 1);

    for (auto& resource : m_resources)
    {
        auto& references = resource->get_references();

        rdg_reference* prev_reference = nullptr;
        for (std::size_t i = 0; i < references.size(); ++i)
        {
            rdg_reference* next_reference = references[i];
            std::size_t pass_index = next_reference->pass->get_index();

            if (resource->get_type() == RDG_RESOURCE_TEXTURE)
            {
                rdg_texture* texture = static_cast<rdg_texture*>(resource.get());

                if (prev_reference == nullptr && next_reference->type != RDG_REFERENCE_ATTACHMENT)
                {
                    rhi_texture_barrier barrier = {};
                    barrier.texture = texture->get_rhi();
                    barrier.src_access = 0;
                    barrier.dst_access = next_reference->access;
                    barrier.src_layout = texture->get_initial_layout();
                    barrier.dst_layout = next_reference->texture.layout;
                    m_barriers[pass_index].texture_barriers.push_back(barrier);

                    m_barriers[pass_index].src_stages |= RHI_PIPELINE_STAGE_BEGIN;
                    m_barriers[pass_index].dst_stages |= next_reference->stages;
                }

                if (prev_reference != nullptr && prev_reference->type != RDG_REFERENCE_ATTACHMENT &&
                    next_reference->type != RDG_REFERENCE_ATTACHMENT &&
                    prev_reference->texture.layout != next_reference->texture.layout)
                {
                    rhi_texture_barrier barrier = {};
                    barrier.texture = texture->get_rhi();
                    barrier.src_access = prev_reference->access;
                    barrier.dst_access = next_reference->access;
                    barrier.src_layout = prev_reference->texture.layout;
                    barrier.dst_layout = next_reference->texture.layout;
                    m_barriers[pass_index].texture_barriers.push_back(barrier);

                    m_barriers[pass_index].src_stages |= prev_reference->stages;
                    m_barriers[pass_index].dst_stages |= next_reference->stages;
                }
            }
            else
            {
            }

            prev_reference = next_reference;
        }
    }

    for (auto& resource : m_resources)
    {
        if (resource->get_type() == RDG_RESOURCE_TEXTURE)
        {
            rdg_texture* texture = static_cast<rdg_texture*>(resource.get());
            rdg_reference* last_reference = resource->get_references().back();
            std::size_t pass_index = last_reference->pass->get_index() + 1;
            if (last_reference->type != RDG_REFERENCE_ATTACHMENT &&
                last_reference->texture.layout != texture->get_final_layout())
            {
                rhi_texture_barrier barrier = {};
                barrier.texture = texture->get_rhi();
                barrier.src_access = last_reference->access;
                barrier.dst_access = 0;
                barrier.src_layout = last_reference->texture.layout;
                barrier.dst_layout = texture->get_final_layout();
                m_barriers[pass_index].texture_barriers.push_back(barrier);

                m_barriers[pass_index].src_stages |= last_reference->stages;
                m_barriers[pass_index].dst_stages |= RHI_PIPELINE_STAGE_END;
            }
        }
        else
        {
        }
    }
}
} // namespace violet