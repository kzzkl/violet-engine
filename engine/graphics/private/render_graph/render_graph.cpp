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

rdg_texture* render_graph::add_texture(std::string_view name, const rhi_texture_desc& desc)
{
    auto resource = std::make_unique<rdg_inter_texture>(
        desc,
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_UNDEFINED);
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

rdg_buffer* render_graph::add_buffer(std::string_view name, const rhi_buffer_desc& desc)
{
    assert(desc.size > 0);

    auto resource = std::make_unique<rdg_inter_buffer>(desc);
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
            if (!m_resources[i]->is_external())
            {
                rdg_inter_texture* inter_texture = static_cast<rdg_inter_texture*>(texture);

                rhi_texture* rhi = m_allocator->allocate_texture(inter_texture->get_desc());
                inter_texture->set_rhi(rhi);
            }
        }
        else if (m_resources[i]->get_type() == RDG_RESOURCE_BUFFER)
        {
            if (!m_resources[i]->is_external())
            {
                rdg_inter_buffer* inter_buffer =
                    static_cast<rdg_inter_buffer*>(m_resources[i].get());

                rhi_buffer* rhi = m_allocator->allocate_buffer(inter_buffer->get_desc());
                inter_buffer->set_rhi(rhi);
            }
        }
    }

    merge_pass();
    build_barriers();
}

void render_graph::record(rhi_command* command)
{
    rdg_command cmd(command, m_allocator);

    for (auto& batch : m_batches)
    {
        barrier& barrier = batch.barrier;
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

void render_graph::cull()
{
    /*auto iter = std::remove_if(
        m_resources.begin(),
        m_resources.end(),
        [](const std::unique_ptr<rdg_resource>& resource)
        {
            return !resource->is_external() && resource->get_references().empty();
        });

    m_resources.erase(iter, m_resources.end());*/

    for (std::size_t i = 0; i < m_passes.size(); ++i)
    {
        for (auto& reference : m_passes[i]->get_references())
        {
            reference->resource->add_reference(reference.get());
        }

        m_passes[i]->m_index = i;
    }
}

void render_graph::merge_pass()
{
    std::vector<rdg_pass*> passes;

    auto merge_render_pass = [&, this]()
    {
        if (passes.empty())
        {
            return;
        }

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
                final_layout = texture->get_final_layout() == RHI_TEXTURE_LAYOUT_UNDEFINED ?
                                   reference->get_texture_layout() :
                                   texture->get_final_layout();

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
        if (pass->get_type() == RDG_PASS_RENDER)
        {
            if (passes.empty() || check_merge(passes[0], pass.get()))
            {
                passes.push_back(pass.get());
            }
            else
            {
                merge_render_pass();
                passes.push_back(pass.get());
            }
        }
        else
        {
            merge_render_pass();
            render_batch batch = {};
            batch.passes.push_back(pass.get());
            m_batches.push_back(batch);
        }
    }

    merge_render_pass();

    for (std::size_t i = 0; i < m_batches.size(); ++i)
    {
        for (rdg_pass* pass : m_batches[i].passes)
        {
            pass->m_batch = i;
        }
    }
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
            auto& batch_barrier = m_batches[curr_reference->pass->get_index()].barrier;

            if (resource->get_type() == RDG_RESOURCE_TEXTURE)
            {
                rdg_texture* texture = static_cast<rdg_texture*>(resource.get());

                if (curr_reference->type == RDG_REFERENCE_ATTACHMENT)
                {
                    prev_reference = curr_reference;
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
                        .level_count = texture->get_level_count(),
                        .layer = 0,
                        .layer_count = texture->get_layer_count(),
                    };
                    batch_barrier.texture_barriers.push_back(barrier);
                    batch_barrier.src_stages |= RHI_PIPELINE_STAGE_BEGIN;
                    batch_barrier.dst_stages |= curr_reference->stages;
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
                        .level_count = texture->get_level_count(),
                        .layer = 0,
                        .layer_count = texture->get_layer_count(),
                    };
                    batch_barrier.texture_barriers.push_back(barrier);
                    batch_barrier.src_stages |= prev_reference->stages;
                    batch_barrier.dst_stages |= curr_reference->stages;
                }
            }
            else
            {
                rdg_buffer* buffer = static_cast<rdg_buffer*>(resource.get());

                if (prev_reference != nullptr)
                {
                    rhi_buffer_barrier barrier = {
                        .buffer = buffer->get_rhi(),
                        .src_access = prev_reference->access,
                        .dst_access = curr_reference->access,
                        .offset = 0,
                        .size = buffer->get_buffer_size(),
                    };

                    batch_barrier.buffer_barriers.push_back(barrier);
                    batch_barrier.src_stages |= prev_reference->stages;
                    batch_barrier.dst_stages |= curr_reference->stages;
                }
            }

            prev_reference = curr_reference;
        }
    }

    m_batches.push_back({});

    auto& last_barrier = m_batches.back().barrier;
    for (auto& resource : m_resources)
    {
        if (resource->get_type() == RDG_RESOURCE_TEXTURE)
        {
            rdg_texture* texture = static_cast<rdg_texture*>(resource.get());

            rhi_access_flags src_access = 0;
            rhi_access_flags dst_access = 0;

            rhi_texture_layout src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;
            rhi_texture_layout dst_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;

            rhi_pipeline_stage_flags src_stages = 0;
            rhi_pipeline_stage_flags dst_stages = 0;

            if (texture->get_references().empty())
            {
                src_layout = texture->get_initial_layout();
                dst_layout = texture->get_final_layout();
                src_stages |= RHI_PIPELINE_STAGE_BEGIN;
                dst_stages |= RHI_PIPELINE_STAGE_END;
            }
            else
            {
                rdg_reference* last_reference = resource->get_references().back();
                if (last_reference->type != RDG_REFERENCE_ATTACHMENT)
                {
                    src_access = last_reference->access;
                    src_layout = last_reference->texture.layout;
                    dst_layout = texture->get_final_layout();
                    src_stages |= last_reference->stages;
                    dst_stages |= RHI_PIPELINE_STAGE_END;
                }
            }

            if (src_layout != dst_layout && dst_layout != RHI_TEXTURE_LAYOUT_UNDEFINED)
            {
                rhi_texture_barrier barrier = {
                    .texture = texture->get_rhi(),
                    .src_access = src_access,
                    .dst_access = dst_access,
                    .src_layout = src_layout,
                    .dst_layout = dst_layout,
                    .level = 0,
                    .level_count = texture->get_level_count(),
                    .layer = 0,
                    .layer_count = texture->get_layer_count(),
                };
                last_barrier.texture_barriers.push_back(barrier);
                last_barrier.src_stages |= src_stages;
                last_barrier.dst_stages |= dst_stages;
            }
        }
        else
        {
        }
    }
}
} // namespace violet