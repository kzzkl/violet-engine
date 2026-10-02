#include "graphics/resources/persistent_texture.hpp"
#include "gpu_buffer_uploader.hpp"
#include <array>

namespace violet
{
persistent_texture::persistent_texture(
    const rhi_extent& initial_extent,
    rhi_format format,
    rhi_buffer_flags flags)
{
    set_texture({
        .extent = initial_extent,
        .format = format,
        .flags = flags | RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_TRANSFER_DST,
        .level_count = 1,
        .layer_count = 1,
        .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
    });

    m_requested_extent = initial_extent;
}

void persistent_texture::copy(const void* data, std::size_t size, rhi_texture_region region)
{
    assert(size > 0);

    m_requested_extent.width =
        std::max(m_requested_extent.width, region.offset_x + region.extent.width);
    m_requested_extent.height =
        std::max(m_requested_extent.height, region.offset_y + region.extent.height);
    m_requested_extent.depth =
        std::max(m_requested_extent.depth, region.offset_z + region.extent.depth);

    m_copy_queue.emplace_back(
        copy_command{
            .data = data,
            .size = size,
            .region = region,
        });
}

void persistent_texture::upload(
    gpu_buffer_uploader* uploader,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access,
    rhi_texture_layout layout)
{
    rhi_extent extent = get_rhi()->get_extent();
    if (extent.width < m_requested_extent.width || extent.height < m_requested_extent.height ||
        extent.depth < m_requested_extent.depth)
    {
        reserve(stages, access, layout);
    }

    rhi_texture* texture = get_rhi();
    for (const auto& command : m_copy_queue)
    {
        uploader
            ->upload(texture, command.data, command.size, command.region, stages, access, layout);
    }

    m_copy_queue.clear();
}

void persistent_texture::reserve(
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access,
    rhi_texture_layout layout)
{
    rhi_texture* old_texture = get_rhi();

    rhi_extent new_texture_extent = old_texture->get_extent();
    while (new_texture_extent.width < m_requested_extent.width)
    {
        new_texture_extent.width *= 2;
    }

    while (new_texture_extent.height < m_requested_extent.height)
    {
        new_texture_extent.height *= 2;
    }

    while (new_texture_extent.depth < m_requested_extent.depth)
    {
        new_texture_extent.depth *= 2;
    }

    auto& device = render_device::instance();

    auto new_texture = device.create_texture({
        .extent = new_texture_extent,
        .format = old_texture->get_format(),
        .flags = old_texture->get_flags(),
        .level_count = old_texture->get_level_count(),
        .layer_count = old_texture->get_layer_count(),
        .samples = old_texture->get_samples(),
    });

    rhi_command* command = device.allocate_command();

    std::array<rhi_texture_barrier, 2> barriers;
    barriers[0] = {
        .texture = new_texture.get(),
        .src_stages = RHI_PIPELINE_STAGE_NONE,
        .src_access = RHI_ACCESS_NONE,
        .src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_WRITE,
        .dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .level = 0,
        .level_count = new_texture->get_level_count(),
        .layer = 0,
        .layer_count = new_texture->get_layer_count(),
    };

    barriers[1] = {
        .texture = old_texture,
        .src_stages = stages,
        .src_access = access,
        .src_layout = layout,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_READ,
        .dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_SRC,
        .level = 0,
        .level_count = old_texture->get_level_count(),
        .layer = 0,
        .layer_count = old_texture->get_layer_count(),
    };

    command->set_pipeline_barrier(
        nullptr,
        0,
        barriers.data(),
        static_cast<std::uint32_t>(barriers.size()));

    rhi_texture_region region = {
        .extent = old_texture->get_extent(),
        .level = 0,
        .layer = 0,
        .layer_count = old_texture->get_layer_count(),
    };

    command->copy_texture(old_texture, region, new_texture.get(), region);

    barriers[0] = {
        .texture = new_texture.get(),
        .src_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = RHI_ACCESS_TRANSFER_WRITE,
        .src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .dst_stages = stages,
        .dst_access = access,
        .dst_layout = layout,
        .level = 0,
        .level_count = new_texture->get_level_count(),
        .layer = 0,
        .layer_count = new_texture->get_layer_count(),
    };

    command->set_pipeline_barrier(nullptr, 0, barriers.data(), 1);

    device.execute_sync(command);

    set_texture(std::move(new_texture));
}
} // namespace violet