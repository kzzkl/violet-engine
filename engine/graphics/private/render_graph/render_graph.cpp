#include "graphics/render_graph/render_graph.hpp"
#include <cassert>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>

namespace violet
{
render_graph::render_graph(rdg_allocator* allocator) noexcept
    : m_allocator(allocator)
{
    m_groups.push_back({});
}

render_graph::~render_graph() {}

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

void render_graph::begin_group(std::string_view group_name)
{
    m_groups[m_passes.size()].push_back(group_name.data());
}

void render_graph::end_group()
{
    m_groups[m_passes.size()].push_back("");
}

void render_graph::compile()
{
    cull();

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
                    .texture = texture_view->m_rhi_texture ? texture_view->m_rhi_texture :
                                                             texture_view->m_rdg_texture->m_texture,
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
        {
            reference->resource->add_reference(reference.get());
        }

        m_passes[i]->m_index = i;
    }

    m_barriers.resize(m_passes.size() + 1);

    merge_pass();
    build_barriers();
}

void render_graph::execute(rhi_command* command)
{
    rdg_command cmd(command, m_allocator);

    for (auto& batch : m_batches)
    {
        for (rdg_pass* pass : batch.passes)
        {
#ifndef NDEBUG
            for (auto& group : m_groups[pass->get_index()])
            {
                if (group.empty())
                {
                    command->end_label();
                }
                else
                {
                    command->begin_label(group.data());
                }
            }

            command->begin_label(pass->get_name().c_str());
#endif

            barrier& barrier = m_barriers[pass->get_index()];
            if (!barrier.texture_barriers.empty() || !barrier.buffer_barriers.empty())
            {
                command->set_pipeline_barrier(
                    barrier.src_stages,
                    barrier.dst_stages,
                    barrier.buffer_barriers.data(),
                    barrier.buffer_barriers.size(),
                    barrier.texture_barriers.data(),
                    barrier.texture_barriers.size());
            }

            if (batch.passes.front() == pass && batch.render_pass != nullptr)
            {
                command->begin_render_pass(batch.render_pass, batch.framebuffer);
                cmd.m_render_pass = batch.render_pass;
                cmd.m_subpass_index = 0;
            }

            pass->execute(cmd);

            if (batch.passes.back() == pass && batch.render_pass != nullptr)
            {
                command->end_render_pass();
                cmd.m_render_pass = nullptr;
                cmd.m_subpass_index = 0;
            }

#ifndef NDEBUG
            command->end_label();
#endif
        }
    }

#ifndef NDEBUG
    for (auto& group : m_groups.back())
    {
        if (group.empty())
        {
            command->end_label();
        }
        else
        {
            command->begin_label(group.data());
        }
    }
#endif
}

void render_graph::cull() {}

void render_graph::merge_pass()
{
    std::vector<rdg_pass*> passes;

    auto merge_batch = [&, this]()
    {
        if (passes.empty())
            return;

        render_batch batch = {};
        batch.passes = passes;

        rhi_render_pass_desc render_pass_desc = {};
        rhi_framebuffer_desc framebuffer_desc = {};

        auto& begin_dependency = render_pass_desc.dependencies[0];
        begin_dependency.src = RHI_RENDER_SUBPASS_EXTERNAL;
        begin_dependency.dst = 0;

        auto& end_dependency = render_pass_desc.dependencies[1];
        end_dependency.src = 0;
        end_dependency.dst = RHI_RENDER_SUBPASS_EXTERNAL;

        render_pass_desc.dependency_count = 2;

        auto& first_pass_attachemets =
            static_cast<rdg_render_pass*>(passes.front())->get_attachments();
        auto& last_pass_attachemets =
            static_cast<rdg_render_pass*>(passes.back())->get_attachments();
        for (std::size_t i = 0; i < first_pass_attachemets.size(); ++i)
        {
            rdg_reference* reference = first_pass_attachemets[i];

            rdg_texture* texture = static_cast<rdg_texture*>(reference->resource);

            rhi_texture_layout initial_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;
            rhi_texture_layout final_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;

            if (reference->is_first_reference())
            {
                initial_layout = texture->get_initial_layout();

                begin_dependency.src_stages |= RHI_PIPELINE_STAGE_END;
            }
            else
            {
                rdg_reference* prev_reference = reference->get_prev_reference();
                initial_layout = prev_reference->get_texture_layout();

                begin_dependency.src_access |= prev_reference->access;
                begin_dependency.src_stages |= prev_reference->stages;
            }

            if (last_pass_attachemets[i]->is_last_reference())
            {
                final_layout = texture->get_final_layout();

                end_dependency.dst_stages |= RHI_PIPELINE_STAGE_BEGIN;
            }
            else
            {
                rdg_reference* next_reference = last_pass_attachemets[i]->get_next_reference();
                final_layout = next_reference->get_texture_layout();

                end_dependency.dst_access |= next_reference->access;
                end_dependency.dst_stages |= next_reference->stages;
            }

            begin_dependency.dst_access |= reference->access;
            begin_dependency.dst_stages |= reference->stages;
            end_dependency.src_access |= reference->access;
            end_dependency.src_stages |= reference->stages;

            render_pass_desc.attachments[render_pass_desc.attachment_count++] = {
                .format = texture->get_format(),
                .samples = RHI_SAMPLE_COUNT_1,
                .initial_layout = initial_layout,
                .final_layout = final_layout,
                .load_op = reference->attachment.load_op,
                .store_op = reference->attachment.store_op,
                .stencil_load_op = reference->attachment.load_op,
                .stencil_store_op = reference->attachment.store_op};

            auto& subpass = render_pass_desc.subpasses[0];
            subpass.references[subpass.reference_count] = {
                .type = reference->attachment.type,
                .layout = reference->get_texture_layout(),
                .index = subpass.reference_count};
            ++subpass.reference_count;

            framebuffer_desc.attachments[i] = texture->get_rhi();
        }
        render_pass_desc.subpass_count = 1;

        batch.render_pass = m_allocator->get_render_pass(render_pass_desc);

        framebuffer_desc.render_pass = batch.render_pass;
        batch.framebuffer = m_allocator->get_framebuffer(framebuffer_desc);

        m_batches.push_back(batch);

        passes.clear();
    };

    auto check_merge = [](rdg_pass* a, rdg_pass* b) -> bool
    {
        auto& a_attachments = static_cast<rdg_render_pass*>(a)->get_attachments();
        auto& b_attachments = static_cast<rdg_render_pass*>(b)->get_attachments();

        if (a_attachments.size() != b_attachments.size())
        {
            return false;
        }

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
            merge_batch();
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
            merge_batch();
            passes.push_back(pass.get());
        }
    }

    merge_batch();
}

void render_graph::build_barriers()
{
    for (auto& resource : m_resources)
    {
        auto& references = resource->get_references();

        rdg_reference* prev_reference = nullptr;
        for (std::size_t i = 0; i < references.size(); ++i)
        {
            rdg_reference* curr_reference = references[i];
            std::size_t pass_index = curr_reference->pass->get_index();

            if (resource->get_type() == RDG_RESOURCE_TEXTURE)
            {
                rdg_texture* texture = static_cast<rdg_texture*>(resource.get());

                if (curr_reference->type == RDG_REFERENCE_ATTACHMENT)
                {
                    continue;
                }

                if (prev_reference == nullptr)
                {
                    rhi_texture_barrier barrier = {
                        .texture = texture->get_rhi(),
                        .src_access = 0,
                        .dst_access = curr_reference->access,
                        .src_layout = texture->get_initial_layout(),
                        .dst_layout = curr_reference->texture.layout,
                        .level = 0,
                        .level_count = 1,
                        .layer = 0,
                        .layer_count = 1};
                    m_barriers[pass_index].texture_barriers.push_back(barrier);
                    m_barriers[pass_index].src_stages |= RHI_PIPELINE_STAGE_BEGIN;
                    m_barriers[pass_index].dst_stages |= curr_reference->stages;
                }
                else if (prev_reference->type != RDG_REFERENCE_ATTACHMENT)
                {
                    rhi_texture_barrier barrier = {
                        .texture = texture->get_rhi(),
                        .src_access = prev_reference->access,
                        .dst_access = curr_reference->access,
                        .src_layout = prev_reference->texture.layout,
                        .dst_layout = curr_reference->texture.layout,
                        .level = 0,
                        .level_count = 1,
                        .layer = 0,
                        .layer_count = 1};
                    m_barriers[pass_index].texture_barriers.push_back(barrier);
                    m_barriers[pass_index].src_stages |= prev_reference->stages;
                    m_barriers[pass_index].dst_stages |= curr_reference->stages;
                }
            }
            else
            {
            }

            prev_reference = curr_reference;
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
                rhi_texture_barrier barrier = {
                    .texture = texture->get_rhi(),
                    .src_access = last_reference->access,
                    .dst_access = 0,
                    .src_layout = last_reference->texture.layout,
                    .dst_layout = texture->get_final_layout(),
                    .level = 0,
                    .level_count = 1,
                    .layer = 0,
                    .layer_count = 1};
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