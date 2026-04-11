#include "graphics/texture_loader.hpp"
#include "graphics/dds.hpp"
#include <cstddef>

namespace violet
{
rhi_ptr<rhi_texture> texture_loader::load(const texture_data& data, load_options options)
{
    rhi_extent extent = data.extent;

    rhi_texture_desc texture_desc = {
        .extent = extent,
        .format = data.format,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST,
        .level_count = data.level_count,
        .layer_count = data.layer_count,
        .samples = RHI_SAMPLE_COUNT_1,
    };

    if (options & LOAD_OPTION_CUBE_MAP)
    {
        texture_desc.flags |= RHI_TEXTURE_CUBE;
    }

    rhi_ptr<rhi_texture> texture = render_device::instance().create_texture(texture_desc);

    rhi_command* command = render_device::instance().allocate_command();

    upload(command, data, texture.get());

    rhi_texture_barrier texture_barrier = {
        .texture = texture.get(),
        .src_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = RHI_ACCESS_TRANSFER_WRITE,
        .src_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .dst_stages =
            RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT | RHI_PIPELINE_STAGE_COMPUTE,
        .dst_access = RHI_ACCESS_SHADER_READ,
        .dst_layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        .level = 0,
        .level_count = data.level_count,
        .layer = 0,
        .layer_count = data.layer_count,
    };

    command->set_pipeline_barrier(nullptr, 0, &texture_barrier, 1);

    render_device::instance().execute_sync(command);

    return texture;
}

rhi_ptr<rhi_texture> texture_loader::load(std::string_view path)
{
    assert(path.ends_with(".dds"));

    texture_data data;
    if (!dds::load(path, data))
    {
        return nullptr;
    }

    return load(data);
}

void texture_loader::upload(rhi_command* command, const texture_data& data, rhi_texture* texture)
{
    rhi_ptr<rhi_buffer> staging_buffer = render_device::instance().create_buffer({
        .size = data.pixels.size(),
        .flags = RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_HOST_VISIBLE,
    });

    auto* buffer = static_cast<std::uint8_t*>(staging_buffer->get_buffer_pointer());
    std::memcpy(buffer, data.pixels.data(), data.pixels.size());

    rhi_buffer_barrier buffer_barrier = {
        .buffer = staging_buffer.get(),
        .src_stages = RHI_PIPELINE_STAGE_HOST,
        .src_access = RHI_ACCESS_HOST_WRITE,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_READ,
        .size = data.pixels.size(),
    };

    rhi_texture_barrier texture_barrier = {
        .texture = texture,
        .src_stages = RHI_PIPELINE_STAGE_HOST | RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = 0,
        .src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED,
        .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .dst_access = RHI_ACCESS_TRANSFER_WRITE,
        .dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
        .level_count = data.level_count,
        .layer_count = data.layer_count,
    };

    std::vector<rhi_buffer_texture_copy> copy_regions;

    rhi_format_size format_size = rhi_get_format_size(data.format);
    std::size_t buffer_offset = 0;

    for (std::uint32_t layer = 0; layer < data.layer_count; ++layer)
    {
        for (std::uint32_t level = 0; level < data.level_count; ++level)
        {
            std::uint32_t width = std::max(data.extent.width >> level, 1u);
            std::uint32_t height = std::max(data.extent.height >> level, 1u);

            auto block_count_x = static_cast<std::uint32_t>(
                std::ceil(static_cast<float>(width) / static_cast<float>(format_size.block_width)));
            block_count_x = std::max(block_count_x, 1u);

            auto block_count_y = static_cast<std::uint32_t>(std::ceil(
                static_cast<float>(height) / static_cast<float>(format_size.block_height)));
            block_count_y = std::max(block_count_y, 1u);

            std::uint32_t buffer_size = block_count_x * block_count_y * format_size.block_size;

            rhi_buffer_texture_copy copy_region = {
                .buffer_region =
                    {
                        .offset = buffer_offset,
                        .size = buffer_size,
                    },
                .texture_region = {
                    .extent = {.width = width, .height = height, .depth = 1},
                    .level = level,
                    .layer = layer,
                    .layer_count = 1,
                }};

            buffer_offset += buffer_size;

            copy_regions.push_back(copy_region);
        }
    }

    command->set_pipeline_barrier(&buffer_barrier, 1, &texture_barrier, 1);

    command->copy_buffer_to_texture(
        staging_buffer.get(),
        texture,
        copy_regions.data(),
        static_cast<std::uint32_t>(copy_regions.size()));
}
} // namespace violet