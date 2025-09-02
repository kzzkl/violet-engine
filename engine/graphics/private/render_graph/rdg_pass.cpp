#include "graphics/render_graph/rdg_pass.hpp"
#include "graphics/render_device.hpp"
#include <cassert>

namespace violet
{
rdg_pass::rdg_pass(rdg_allocator* allocator) noexcept
    : m_allocator(allocator)
{
}

rdg_texture_ref rdg_pass::add_texture(
    rdg_texture* texture,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access,
    rhi_texture_layout layout,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_TEXTURE;
    reference->pass = this;
    reference->resource = texture;
    reference->stages = stages;
    reference->access = access;
    reference->texture.layout = layout;
    reference->texture.level = level;
    reference->texture.level_count = level_count == 0 ? texture->get_level_count() : level_count;
    reference->texture.layer = layer;
    reference->texture.layer_count = layer_count == 0 ? texture->get_layer_count() : layer_count;

    m_references.push_back(reference);

    return rdg_texture_ref(reference);
}

rdg_texture_srv rdg_pass::add_texture_srv(
    rdg_texture* texture,
    rhi_pipeline_stage_flags stages,
    rhi_texture_dimension dimension,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    assert(texture->get_flags() & RHI_TEXTURE_SHADER_RESOURCE);

    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_TEXTURE_SRV;
    reference->pass = this;
    reference->resource = texture;
    reference->stages = stages;
    reference->access = RHI_ACCESS_SHADER_READ;
    reference->texture.layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE;
    reference->texture.level = level;
    reference->texture.level_count = level_count == 0 ? texture->get_level_count() : level_count;
    reference->texture.layer = layer;
    reference->texture.layer_count = layer_count == 0 ? texture->get_layer_count() : layer_count;
    reference->texture.srv.dimension = dimension;

    m_references.push_back(reference);

    return rdg_texture_srv(reference);
}

rdg_texture_uav rdg_pass::add_texture_uav(
    rdg_texture* texture,
    rhi_pipeline_stage_flags stages,
    rhi_texture_dimension dimension,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    assert(texture->get_flags() & RHI_TEXTURE_STORAGE);

    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_TEXTURE_UAV;
    reference->pass = this;
    reference->resource = texture;
    reference->stages = stages;
    reference->access = RHI_ACCESS_SHADER_READ | RHI_ACCESS_SHADER_WRITE;
    reference->texture.layout = RHI_TEXTURE_LAYOUT_GENERAL;
    reference->texture.level = level;
    reference->texture.level_count = level_count == 0 ? texture->get_level_count() : level_count;
    reference->texture.layer = layer;
    reference->texture.layer_count = layer_count == 0 ? texture->get_layer_count() : layer_count;
    reference->texture.uav.dimension = dimension;

    m_references.push_back(reference);

    return rdg_texture_uav(reference);
}

rdg_buffer_ref rdg_pass::add_buffer(
    rdg_buffer* buffer,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access,
    std::uint64_t offset,
    std::uint64_t size)
{
    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_BUFFER;
    reference->pass = this;
    reference->resource = buffer;
    reference->stages = stages;
    reference->access = access;
    reference->buffer.offset = offset;
    reference->buffer.size = size == 0 ? buffer->get_size() : size;

    m_references.push_back(reference);

    return rdg_buffer_ref(reference);
}

rdg_buffer_srv rdg_pass::add_buffer_srv(
    rdg_buffer* buffer,
    rhi_pipeline_stage_flags stages,
    std::uint64_t offset,
    std::uint64_t size,
    rhi_format texel_format)
{
    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_BUFFER_SRV;
    reference->pass = this;
    reference->resource = buffer;
    reference->stages = stages;
    reference->access = RHI_ACCESS_SHADER_READ;
    reference->buffer.offset = offset;
    reference->buffer.size = size == 0 ? buffer->get_size() : size;
    reference->buffer.srv.texel_format = texel_format;

    m_references.push_back(reference);

    return rdg_buffer_srv(reference);
}

rdg_buffer_uav rdg_pass::add_buffer_uav(
    rdg_buffer* buffer,
    rhi_pipeline_stage_flags stages,
    std::uint64_t offset,
    std::uint64_t size,
    rhi_format texel_format)
{
    assert(
        buffer->get_flags() & RHI_BUFFER_STORAGE || buffer->get_flags() & RHI_BUFFER_STORAGE_TEXEL);

    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_BUFFER_UAV;
    reference->pass = this;
    reference->resource = buffer;
    reference->stages = stages;
    reference->access = RHI_ACCESS_SHADER_READ | RHI_ACCESS_SHADER_WRITE;
    reference->buffer.offset = offset;
    reference->buffer.size = size == 0 ? buffer->get_size() : size;
    reference->buffer.uav.texel_format = texel_format;

    m_references.push_back(reference);

    return rdg_buffer_uav(reference);
}

rdg_texture_rtv rdg_pass::add_render_target(
    rdg_texture* texture,
    rhi_attachment_load_op load_op,
    rhi_attachment_store_op store_op,
    std::uint32_t level,
    std::uint32_t layer,
    rhi_clear_value clear_value)
{
    assert(texture->get_flags() & RHI_TEXTURE_RENDER_TARGET);

    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_TEXTURE_RTV;
    reference->pass = this;
    reference->resource = texture;
    reference->stages = RHI_PIPELINE_STAGE_COLOR_OUTPUT;
    reference->access = load_op == RHI_ATTACHMENT_LOAD_OP_LOAD ?
                            RHI_ACCESS_COLOR_READ | RHI_ACCESS_COLOR_WRITE :
                            RHI_ACCESS_COLOR_WRITE;
    reference->texture.layout = RHI_TEXTURE_LAYOUT_RENDER_TARGET;
    reference->texture.level = level;
    reference->texture.level_count = 1;
    reference->texture.layer = layer;
    reference->texture.layer_count = 1;
    reference->texture.rtv.load_op = load_op;
    reference->texture.rtv.store_op = store_op;
    reference->texture.rtv.clear_value = clear_value;

    m_references.push_back(reference);

    return rdg_texture_rtv(reference);
}

rdg_texture_dsv rdg_pass::set_depth_stencil(
    rdg_texture* texture,
    rhi_attachment_load_op load_op,
    rhi_attachment_store_op store_op,
    std::uint32_t level,
    std::uint32_t layer,
    rhi_clear_value clear_value)
{
    assert(texture->get_flags() & RHI_TEXTURE_DEPTH_STENCIL);

    auto* reference = m_allocator->allocate_reference();
    reference->type = RDG_REFERENCE_TEXTURE_DSV;
    reference->pass = this;
    reference->resource = texture;
    reference->stages =
        RHI_PIPELINE_STAGE_EARLY_DEPTH_STENCIL | RHI_PIPELINE_STAGE_LATE_DEPTH_STENCIL;
    reference->access = RHI_ACCESS_DEPTH_STENCIL_READ | RHI_ACCESS_DEPTH_STENCIL_WRITE;
    reference->texture.layout = RHI_TEXTURE_LAYOUT_DEPTH_STENCIL;
    reference->texture.level = level;
    reference->texture.level_count = 1;
    reference->texture.layer = layer;
    reference->texture.layer_count = 1;
    reference->texture.dsv.load_op = load_op;
    reference->texture.dsv.store_op = store_op;
    reference->texture.dsv.clear_value = clear_value;

    m_references.push_back(reference);

    return rdg_texture_dsv(reference);
}

rhi_parameter* rdg_pass::add_parameter(const rhi_parameter_desc& desc)
{
    return render_device::instance().allocate_parameter(desc);
}

void rdg_pass::execute(rdg_command& command)
{
    on_execute(command);
}
} // namespace violet