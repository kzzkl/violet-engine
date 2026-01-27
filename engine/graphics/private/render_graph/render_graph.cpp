#include "graphics/render_graph/render_graph.hpp"
#include <algorithm>
#include <cassert>

namespace violet
{
namespace
{
struct render_pass
{
    rdg_pass* pass;
    std::vector<rdg_reference*> attachments;

    bool is_mergeable(const render_pass& other) const noexcept
    {
        if (attachments.size() != other.attachments.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < attachments.size(); ++i)
        {
            if (attachments[i]->resource != other.attachments[i]->resource ||
                !is_slice_equal(attachments[i], other.attachments[i]))
            {
                return false;
            }

            if ((other.attachments[i]->type == RDG_REFERENCE_TEXTURE_RTV &&
                 other.attachments[i]->texture.rtv.load_op == RHI_ATTACHMENT_LOAD_OP_CLEAR) ||
                (other.attachments[i]->type == RDG_REFERENCE_TEXTURE_DSV &&
                 other.attachments[i]->texture.dsv.load_op == RHI_ATTACHMENT_LOAD_OP_CLEAR))
            {
                return false;
            }
        }

        if (attachments.empty())
        {
            return pass->get_render_area() == other.pass->get_render_area();
        }

        return true;
    }

    static bool is_slice_equal(const rdg_reference* a, const rdg_reference* b) noexcept
    {
        assert(
            a->type == RDG_REFERENCE_TEXTURE || a->type == RDG_REFERENCE_TEXTURE_SRV ||
            a->type == RDG_REFERENCE_TEXTURE_UAV || a->type == RDG_REFERENCE_TEXTURE_RTV ||
            a->type == RDG_REFERENCE_TEXTURE_DSV);
        assert(
            b->type == RDG_REFERENCE_TEXTURE || b->type == RDG_REFERENCE_TEXTURE_SRV ||
            b->type == RDG_REFERENCE_TEXTURE_UAV || b->type == RDG_REFERENCE_TEXTURE_RTV ||
            b->type == RDG_REFERENCE_TEXTURE_DSV);

        return a->texture.level == b->texture.level &&
               a->texture.level_count == b->texture.level_count &&
               a->texture.layer == b->texture.layer &&
               a->texture.layer_count == b->texture.layer_count;
    }
};

struct texture_slice
{
    vec2u a;
    vec2u b;

    const rdg_reference* reference;

    std::uint32_t get_level() const noexcept
    {
        return a.y;
    }

    std::uint32_t get_level_count() const noexcept
    {
        return b.y - a.y;
    }

    std::uint32_t get_layer() const noexcept
    {
        return a.x;
    }

    std::uint32_t get_layer_count() const noexcept
    {
        return b.x - a.x;
    }

    std::uint32_t get_area() const
    {
        return (b.x - a.x) * (b.y - a.y);
    }

    std::uint32_t get_overlap_area(const texture_slice& other) const
    {
        vec2u p0 = {std::max(a.x, other.a.x), std::max(a.y, other.a.y)};
        vec2u p1 = {std::min(b.x, other.b.x), std::min(b.y, other.b.y)};

        return (p1.x - p0.x) * (p1.y - p0.y);
    }

    bool is_equal(const texture_slice& other) const noexcept
    {
        return a == other.a && b == other.b;
    }

    bool is_overlap(const texture_slice& other) const noexcept
    {
        if (b.x <= other.a.x || other.b.x <= a.x)
        {
            return false;
        }

        if (b.y <= other.a.y || other.b.y <= a.y)
        {
            return false;
        }

        return true;
    }

    texture_slice subtract(const texture_slice& other, std::vector<texture_slice>& remaining_slices)
        const
    {
        if (other.a.x > a.x)
        {
            texture_slice left_slice = {
                .a = a,
                .b = {other.a.x, b.y},
                .reference = reference,
            };
            remaining_slices.push_back(left_slice);
        }

        if (other.b.x < b.x)
        {
            texture_slice right_slice = {
                .a = {other.b.x, a.y},
                .b = b,
                .reference = reference,
            };
            remaining_slices.push_back(right_slice);
        }

        if (other.a.y > a.y)
        {
            texture_slice top_slice = {
                .a = {std::max(other.a.x, a.x), a.y},
                .b = {std::min(other.b.x, b.x), other.a.y},
                .reference = reference,
            };
            remaining_slices.push_back(top_slice);
        }

        if (other.b.y < b.y)
        {
            texture_slice bottom_slice = {
                .a = {std::max(other.a.x, a.x), other.b.y},
                .b = {std::min(other.b.x, b.x), b.y},
                .reference = reference,
            };
            remaining_slices.push_back(bottom_slice);
        }

        return {
            .a = {std::max(a.x, other.a.x), std::max(a.y, other.a.y)},
            .b = {std::min(b.x, other.b.x), std::min(b.y, other.b.y)},
            .reference = reference,
        };
    }
};

bool is_first_reference(const rdg_reference* reference) noexcept
{
    return reference->resource->get_references().front() == reference;
}

bool is_last_reference(const rdg_reference* reference) noexcept
{
    return reference->resource->get_references().back() == reference;
}

rdg_reference* get_prev_reference(const rdg_reference* reference)
{
    assert(!is_first_reference(reference));
    return reference->resource->get_references()[reference->index - 1];
}

rdg_reference* get_next_reference(const rdg_reference* reference)
{
    assert(!is_last_reference(reference));
    return reference->resource->get_references()[reference->index + 1];
}

void add_attachment(
    rhi_render_pass_desc& render_pass_desc,
    const rdg_reference* first_reference,
    const rdg_reference* last_reference)
{
    assert(first_reference->type == last_reference->type);
    assert(
        first_reference->type == RDG_REFERENCE_TEXTURE_RTV ||
        first_reference->type == RDG_REFERENCE_TEXTURE_DSV);

    const auto* texture = static_cast<const rdg_texture*>(first_reference->resource);

    rhi_texture_layout initial_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;
    rhi_texture_layout final_layout = RHI_TEXTURE_LAYOUT_UNDEFINED;

    auto& begin_dependency = render_pass_desc.begin_dependency;
    auto& end_dependency = render_pass_desc.end_dependency;

    if (is_first_reference(first_reference))
    {
        initial_layout = texture->get_initial_layout();
        begin_dependency.src_stages |= RHI_PIPELINE_STAGE_END;
    }
    else
    {
        const auto* prev_reference = get_prev_reference(first_reference);
        initial_layout = prev_reference->texture.layout;

        begin_dependency.src_access |= prev_reference->access;
        begin_dependency.src_stages |= prev_reference->stages;
    }
    begin_dependency.dst_access |= first_reference->access;
    begin_dependency.dst_stages |= first_reference->stages;

    if (is_last_reference(last_reference))
    {
        final_layout = texture->get_final_layout() == RHI_TEXTURE_LAYOUT_UNDEFINED ?
                           last_reference->texture.layout :
                           texture->get_final_layout();

        end_dependency.dst_stages |= RHI_PIPELINE_STAGE_BEGIN;
    }
    else
    {
        const auto* next_reference = get_next_reference(last_reference);
        final_layout = next_reference->texture.layout;

        end_dependency.dst_access |= next_reference->access;
        end_dependency.dst_stages |= next_reference->stages;
    }
    end_dependency.src_access |= first_reference->access;
    end_dependency.src_stages |= first_reference->stages;

    rhi_attachment_load_op load_op = first_reference->type == RDG_REFERENCE_TEXTURE_RTV ?
                                         first_reference->texture.rtv.load_op :
                                         first_reference->texture.dsv.load_op;
    rhi_attachment_store_op store_op = last_reference->type == RDG_REFERENCE_TEXTURE_RTV ?
                                           last_reference->texture.rtv.store_op :
                                           last_reference->texture.dsv.store_op;

    render_pass_desc.attachments[render_pass_desc.attachment_count] = {
        .type = first_reference->type == RDG_REFERENCE_TEXTURE_DSV ?
                    RHI_ATTACHMENT_TYPE_DEPTH_STENCIL :
                    RHI_ATTACHMENT_TYPE_RENDER_TARGET,
        .format = texture->get_format(),
        .samples = texture->get_samples(),
        .layout = first_reference->texture.layout,
        .initial_layout = initial_layout,
        .final_layout = final_layout,
        .load_op = load_op,
        .store_op = store_op,
        .stencil_load_op = load_op,
        .stencil_store_op = store_op,
    };
    ++render_pass_desc.attachment_count;
};

bool is_read_access(rhi_access_flags access) noexcept
{
    static constexpr rhi_access_flags read_access =
        RHI_ACCESS_COLOR_READ | RHI_ACCESS_DEPTH_STENCIL_READ | RHI_ACCESS_SHADER_READ |
        RHI_ACCESS_TRANSFER_READ | RHI_ACCESS_HOST_READ | RHI_ACCESS_INDIRECT_COMMAND_READ |
        RHI_ACCESS_VERTEX_ATTRIBUTE_READ;

    return (access & read_access) == access;
};
} // namespace

render_graph::render_graph(
    std::string_view name,
    const render_scene* scene,
    const render_camera* camera,
    rdg_allocator* allocator) noexcept
    : m_allocator(allocator),
      m_scene(scene),
      m_camera(camera)
{
    if (m_allocator == nullptr)
    {
        m_default_allocator = std::make_unique<rdg_allocator>();
        m_allocator = m_default_allocator.get();
    }

    m_final_pass = m_allocator->allocate_pass<rdg_pass>();
    m_final_pass->set_name("Final");
    m_final_pass->set_pass_type(RDG_PASS_TRANSFER);

    begin_group(name);
}

render_graph::~render_graph() {}

rdg_texture* render_graph::add_texture(
    std::string_view name,
    rhi_texture* texture,
    rhi_texture_layout initial_layout,
    rhi_texture_layout final_layout)
{
    assert(texture != nullptr);

    auto* resource = m_allocator->allocate_resource<rdg_texture>();
    resource->set_name(name);
    resource->set_external(true);
    resource->set_extent(texture->get_extent());
    resource->set_format(texture->get_format());
    resource->set_flags(texture->get_flags());
    resource->set_level_count(texture->get_level_count());
    resource->set_layer_count(texture->get_layer_count());
    resource->set_samples(texture->get_samples());
    resource->set_initial_layout(initial_layout);
    resource->set_final_layout(final_layout);
    resource->set_rhi(texture);

    m_resources.push_back(resource);

    return resource;
}

rdg_texture* render_graph::add_texture(
    std::string_view name,
    rhi_texture_extent extent,
    rhi_format format,
    rhi_texture_flags flags,
    std::uint32_t level_count,
    std::uint32_t layer_count,
    rhi_sample_count samples)
{
    auto* resource = m_allocator->allocate_resource<rdg_texture>();
    resource->set_name(name);
    resource->set_external(false);
    resource->set_extent(extent);
    resource->set_format(format);
    resource->set_flags(flags);
    resource->set_level_count(level_count);
    resource->set_layer_count(layer_count);
    resource->set_samples(samples);
    resource->set_initial_layout(RHI_TEXTURE_LAYOUT_UNDEFINED);
    resource->set_final_layout(RHI_TEXTURE_LAYOUT_UNDEFINED);
    resource->set_rhi(nullptr);

    m_resources.push_back(resource);

    return resource;
}

rdg_buffer* render_graph::add_buffer(std::string_view name, rhi_buffer* buffer)
{
    assert(buffer != nullptr);

    auto* resource = m_allocator->allocate_resource<rdg_buffer>();
    resource->set_name(name);
    resource->set_external(true);
    resource->set_size(buffer->get_size());
    resource->set_flags(buffer->get_flags());
    resource->set_rhi(buffer);

    m_resources.push_back(resource);

    return resource;
}

rdg_buffer* render_graph::add_buffer(
    std::string_view name,
    std::size_t size,
    rhi_buffer_flags flags)
{
    assert(size > 0);

    auto* resource = m_allocator->allocate_resource<rdg_buffer>();
    resource->set_name(name);
    resource->set_external(false);
    resource->set_size(size);
    resource->set_flags(flags);
    resource->set_rhi(nullptr);

    m_resources.push_back(resource);

    return resource;
}

void render_graph::begin_group(std::string_view group_name)
{
    m_labels.emplace_back(group_name.data());
}

void render_graph::end_group()
{
    m_labels.emplace_back("");
}

void render_graph::compile()
{
    end_group();

    cull();
    allocate_resources();
    merge_passes();
    build_barriers();
}

void render_graph::record(rhi_command* command)
{
    rdg_command cmd(command, m_allocator, m_scene, m_camera);

    std::size_t batch_index = 0;
    std::size_t label_index = 0;

    auto set_label = [&](std::size_t end)
    {
        for (std::size_t i = label_index; i < end; ++i)
        {
            if (!m_labels[i].empty())
            {
                command->begin_label(m_labels[i].c_str());
            }
            else
            {
                command->end_label();
            }
        }

        label_index = end;
    };

    for (std::size_t i = 0; i < m_passes.size(); ++i)
    {
        set_label(m_label_offset[i]);

        rdg_pass* pass = m_passes[i];

        if (pass->is_culled())
        {
            continue;
        }

        command->begin_label(pass->get_name().c_str());

        if (m_batches[batch_index].begin_pass == pass)
        {
            auto& batch = m_batches[batch_index];

            if (!batch.texture_barriers.empty() || !batch.buffer_barriers.empty())
            {
                command->set_pipeline_barrier(
                    batch.buffer_barriers.data(),
                    static_cast<std::uint32_t>(batch.buffer_barriers.size()),
                    batch.texture_barriers.data(),
                    static_cast<std::uint32_t>(batch.texture_barriers.size()));
            }

            if (batch.render_pass != nullptr)
            {
                command->begin_render_pass(
                    batch.render_pass,
                    batch.attachments.data(),
                    static_cast<std::uint32_t>(batch.attachments.size()),
                    batch.begin_pass->get_render_area());
                cmd.m_render_pass = batch.render_pass;
            }
        }
        pass->execute(cmd);

        if (m_batches[batch_index].end_pass == pass)
        {
            auto& batch = m_batches[batch_index];

            if (batch.render_pass != nullptr)
            {
                command->end_render_pass();
                cmd.m_render_pass = nullptr;
            }

            ++batch_index;
        }

        command->end_label();
    }
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

    m_passes.push_back(m_final_pass);
    m_label_offset.push_back(m_labels.size());

    for (auto* resource : m_resources)
    {
        if (resource->is_culled())
        {
            continue;
        }

        if (resource->get_type() == RDG_RESOURCE_TEXTURE)
        {
            auto* texture = static_cast<rdg_texture*>(resource);

            if (texture->get_final_layout() != RHI_TEXTURE_LAYOUT_UNDEFINED)
            {
                m_final_pass
                    ->add_texture(texture, RHI_PIPELINE_STAGE_END, 0, texture->get_final_layout());
            }
        }
    }

    for (auto* pass : m_passes)
    {
        if (pass->is_culled())
        {
            continue;
        }

        pass->each_reference(
            [](rdg_reference* reference)
            {
                reference->index = reference->resource->add_reference(reference);
            });
    }
}

void render_graph::allocate_resources()
{
    auto allocate_rhi = [](rdg_resource* resource)
    {
        if (resource->get_type() == RDG_RESOURCE_TEXTURE)
        {
            auto* texture = static_cast<rdg_texture*>(resource);

            rhi_texture_desc desc = {};
            desc.extent = texture->get_extent();
            desc.format = texture->get_format();
            desc.flags = texture->get_flags();
            desc.level_count = texture->get_level_count();
            desc.layer_count = texture->get_layer_count();
            desc.samples = texture->get_samples();
            texture->set_rhi(render_device::instance().allocate_texture(desc));
        }
        else if (resource->get_type() == RDG_RESOURCE_BUFFER)
        {
            auto* buffer = static_cast<rdg_buffer*>(resource);

            rhi_buffer_desc desc = {};
            desc.size = buffer->get_size();
            desc.flags = buffer->get_flags();
            buffer->set_rhi(render_device::instance().allocate_buffer(desc));
        }
    };

    auto free_rhi = [](rdg_resource* resource)
    {
        if (resource->get_type() == RDG_RESOURCE_TEXTURE)
        {
            auto* texture = static_cast<rdg_texture*>(resource);
            render_device::instance().free_texture(texture->get_rhi());
        }
        else if (resource->get_type() == RDG_RESOURCE_BUFFER)
        {
            auto* buffer = static_cast<rdg_buffer*>(resource);
            render_device::instance().free_buffer(buffer->get_rhi());
        }
    };

    auto update_reference_rhi = [](rdg_reference* reference)
    {
        switch (reference->type)
        {
        case RDG_REFERENCE_TEXTURE_SRV: {
            auto* texture = static_cast<rdg_texture*>(reference->resource);
            reference->texture.srv.rhi = texture->get_rhi()->get_srv(
                reference->texture.srv.dimension,
                reference->texture.level,
                reference->texture.level_count,
                reference->texture.layer,
                reference->texture.layer_count);
            break;
        }
        case RDG_REFERENCE_TEXTURE_UAV: {
            auto* texture = static_cast<rdg_texture*>(reference->resource);
            reference->texture.uav.rhi = texture->get_rhi()->get_uav(
                reference->texture.uav.dimension,
                reference->texture.level,
                reference->texture.level_count,
                reference->texture.layer,
                reference->texture.layer_count);
            break;
        }
        case RDG_REFERENCE_TEXTURE_RTV: {
            auto* texture = static_cast<rdg_texture*>(reference->resource);
            reference->texture.rtv.rhi = texture->get_rhi()->get_rtv(
                RHI_TEXTURE_DIMENSION_2D,
                reference->texture.level,
                reference->texture.level_count,
                reference->texture.layer,
                reference->texture.layer_count);
            break;
        }
        case RDG_REFERENCE_TEXTURE_DSV: {
            auto* texture = static_cast<rdg_texture*>(reference->resource);
            reference->texture.dsv.rhi = texture->get_rhi()->get_dsv(
                RHI_TEXTURE_DIMENSION_2D,
                reference->texture.level,
                reference->texture.level_count,
                reference->texture.layer,
                reference->texture.layer_count);
            break;
        }
        case RDG_REFERENCE_BUFFER_SRV: {
            auto* buffer = static_cast<rdg_buffer*>(reference->resource);
            reference->buffer.srv.rhi = buffer->get_rhi()->get_srv(
                reference->buffer.offset,
                reference->buffer.size,
                reference->buffer.srv.texel_format);
            break;
        }
        case RDG_REFERENCE_BUFFER_UAV: {
            auto* buffer = static_cast<rdg_buffer*>(reference->resource);
            reference->buffer.uav.rhi = buffer->get_rhi()->get_uav(
                reference->buffer.offset,
                reference->buffer.size,
                reference->buffer.uav.texel_format);
            break;
        }
        default:
            break;
        }
    };

    for (auto* pass : m_passes)
    {
        if (pass->is_culled())
        {
            continue;
        }

        pass->each_reference(
            [&](rdg_reference* reference)
            {
                if (!reference->resource->is_external())
                {
                    if (is_first_reference(reference))
                    {
                        allocate_rhi(reference->resource);
                    }
                    else if (is_last_reference(reference))
                    {
                        free_rhi(reference->resource);
                    }
                }

                update_reference_rhi(reference);
            });
    }
}

void render_graph::merge_passes()
{
    std::vector<render_pass> pending_merge_passes;

    auto flush_merge_passes = [&]()
    {
        if (pending_merge_passes.empty())
        {
            return;
        }

        batch batch = {
            .begin_pass = pending_merge_passes.front().pass,
            .end_pass = pending_merge_passes.back().pass,
        };

        rhi_render_pass_desc render_pass_desc = {};

        auto& attachments = pending_merge_passes.front().attachments;
        for (std::size_t i = 0; i < attachments.size(); ++i)
        {
            add_attachment(
                render_pass_desc,
                attachments[i],
                pending_merge_passes.back().attachments[i]);

            if (attachments[i]->type == RDG_REFERENCE_TEXTURE_RTV)
            {
                batch.attachments.push_back({
                    .rtv = attachments[i]->texture.rtv.rhi,
                    .clear_value = attachments[i]->texture.rtv.clear_value,
                });
            }
            else
            {
                batch.attachments.push_back({
                    .dsv = attachments[i]->texture.dsv.rhi,
                    .clear_value = attachments[i]->texture.dsv.clear_value,
                });
            }
        }

        batch.render_pass = render_device::instance().get_render_pass(render_pass_desc);

        m_batches.push_back(batch);

        pending_merge_passes.clear();
    };

    for (auto* pass : m_passes)
    {
        if (pass->is_culled())
        {
            continue;
        }

        pass->set_batch_index(m_batches.size());

        if (pass->get_pass_type() == RDG_PASS_RASTER)
        {
            render_pass info = {
                .pass = pass,
            };

            pass->each_reference(
                [&info](rdg_reference* reference)
                {
                    if (reference->type == RDG_REFERENCE_TEXTURE_RTV ||
                        reference->type == RDG_REFERENCE_TEXTURE_DSV)
                    {
                        info.attachments.push_back(reference);
                    }
                });

            if (!pending_merge_passes.empty() && !pending_merge_passes.back().is_mergeable(info))
            {
                flush_merge_passes();
                pending_merge_passes.push_back(info);
            }
            else
            {
                pending_merge_passes.push_back(info);
            }
        }
        else
        {
            flush_merge_passes();
            m_batches.push_back({
                .begin_pass = pass,
                .end_pass = pass,
            });
        }
    }

    flush_merge_passes();
}

void render_graph::build_barriers()
{
    for (auto& resource : m_resources)
    {
        if (resource->get_type() == RDG_RESOURCE_TEXTURE)
        {
            build_texture_barriers(static_cast<rdg_texture*>(resource));
        }
        else
        {
            build_buffer_barriers(static_cast<rdg_buffer*>(resource));
        }
    }
}

void render_graph::build_texture_barriers(rdg_texture* texture)
{
    std::vector<texture_slice> texture_slices;

    const rdg_reference initial_reference = {
        .type = RDG_REFERENCE_TEXTURE,
        .stages = RHI_PIPELINE_STAGE_END,
        .access = 0,
        .texture =
            {
                .layout = texture->get_initial_layout(),
                .level = 0,
                .level_count = texture->get_level_count(),
                .layer = 0,
                .layer_count = texture->get_layer_count(),
            },
    };

    if (texture->get_initial_layout() != RHI_TEXTURE_LAYOUT_UNDEFINED)
    {
        texture_slices.push_back({
            .a = {0, 0},
            .b = {texture->get_layer_count(), texture->get_level_count()},
            .reference = &initial_reference,
        });
    }

    for (const auto* reference : texture->get_references())
    {
        texture_slice curr_slice = {
            .a = {reference->texture.layer, reference->texture.level},
            .b =
                {
                    reference->texture.layer + reference->texture.layer_count,
                    reference->texture.level + reference->texture.level_count,
                },
            .reference = reference,
        };

        const rdg_reference* curr_reference = reference;
        const rdg_reference* equal_reference = nullptr;

        std::vector<texture_slice> overlap_slices;
        for (auto iter = texture_slices.begin(); iter != texture_slices.end();)
        {
            if (iter->is_equal(curr_slice))
            {
                equal_reference = iter->reference;
                iter->reference = curr_reference;

                break;
            }

            if (iter->is_overlap(curr_slice))
            {
                overlap_slices.push_back(*iter);
                iter = texture_slices.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        if (equal_reference != nullptr)
        {
            add_texture_barrier(
                equal_reference,
                curr_reference,
                curr_slice.get_level(),
                curr_slice.get_level_count(),
                curr_slice.get_layer(),
                curr_slice.get_layer_count());
            continue;
        }

        texture_slices.push_back(curr_slice);

        std::uint32_t overlap_area = 0;
        for (auto& overlap_slice : overlap_slices)
        {
            overlap_area += overlap_slice.get_overlap_area(curr_slice);
        }

        if (overlap_area == curr_slice.get_area())
        {
            for (auto& overlap_slice : overlap_slices)
            {
                auto intersection_slice = overlap_slice.subtract(curr_slice, texture_slices);
                add_texture_barrier(
                    intersection_slice.reference,
                    curr_reference,
                    intersection_slice.get_level(),
                    intersection_slice.get_level_count(),
                    intersection_slice.get_layer(),
                    intersection_slice.get_layer_count());
            }

            continue;
        }

        for (auto& overlap_slice : overlap_slices)
        {
            overlap_slice.subtract(curr_slice, texture_slices);
        }

        add_texture_barrier(
            &initial_reference,
            curr_reference,
            curr_reference->texture.level,
            curr_reference->texture.level_count,
            curr_reference->texture.layer,
            curr_reference->texture.layer_count);
    }
}

void render_graph::build_buffer_barriers(rdg_buffer* buffer)
{
    const rdg_reference* prev_reference = nullptr;
    for (const auto* curr_reference : buffer->get_references())
    {
        if (prev_reference != nullptr)
        {
            add_buffer_barrier(
                prev_reference,
                curr_reference,
                curr_reference->buffer.offset,
                curr_reference->buffer.size);
        }

        prev_reference = curr_reference;
    }
}

void render_graph::add_texture_barrier(
    const rdg_reference* prev_reference,
    const rdg_reference* curr_reference,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    auto skip_attachment = [](const rdg_reference* prev_reference,
                              const rdg_reference* curr_reference) -> bool
    {
        return prev_reference->type == RDG_REFERENCE_TEXTURE_RTV ||
               prev_reference->type == RDG_REFERENCE_TEXTURE_DSV ||
               curr_reference->type == RDG_REFERENCE_TEXTURE_RTV ||
               curr_reference->type == RDG_REFERENCE_TEXTURE_DSV;
    };

    auto skip_readonly = [](const rdg_reference* prev_reference,
                            const rdg_reference* curr_reference) -> bool
    {
        return prev_reference->texture.layout == curr_reference->texture.layout &&
               is_read_access(prev_reference->access) && is_read_access(curr_reference->access);
    };

    if (skip_attachment(prev_reference, curr_reference) ||
        skip_readonly(prev_reference, curr_reference))
    {
        return;
    }

    m_batches[curr_reference->pass->get_batch_index()].texture_barriers.push_back({
        .texture = static_cast<rdg_texture*>(curr_reference->resource)->get_rhi(),
        .src_stages = prev_reference->stages,
        .src_access = prev_reference->access,
        .src_layout = prev_reference->texture.layout,
        .dst_stages = curr_reference->stages,
        .dst_access = curr_reference->access,
        .dst_layout = curr_reference->texture.layout,
        .level = level,
        .level_count = level_count,
        .layer = layer,
        .layer_count = layer_count,
    });
}

void render_graph::add_buffer_barrier(
    const rdg_reference* prev_reference,
    const rdg_reference* curr_reference,
    std::size_t offset,
    std::size_t size)
{
    if (!is_read_access(prev_reference->access) || !is_read_access(curr_reference->access))
    {
        m_batches[curr_reference->pass->get_batch_index()].buffer_barriers.push_back({
            .buffer = static_cast<rdg_buffer*>(curr_reference->resource)->get_rhi(),
            .src_stages = prev_reference->stages,
            .src_access = prev_reference->access,
            .dst_stages = curr_reference->stages,
            .dst_access = curr_reference->access,
            .offset = offset,
            .size = size,
        });
    }
}
} // namespace violet