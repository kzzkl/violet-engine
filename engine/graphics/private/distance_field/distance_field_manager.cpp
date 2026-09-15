#include "distance_field/distance_field_manager.hpp"
#include "gpu_buffer_uploader.hpp"
#include <cstddef>

namespace violet
{
distance_field_manager::distance_field_manager()
{
    m_brick_table =
        std::make_unique<persistent_buffer>(sizeof(std::uint32_t) * 128 * 128, RHI_BUFFER_STORAGE);

    auto& device = render_device::instance();
    m_brick_atlas = device.create_texture({
        .extent =
            {
                .width = 128 * SDF_BRICK_SIZE,
                .height = 1024 * SDF_BRICK_SIZE,
                .depth = SDF_BRICK_SIZE,
            },
        .format = RHI_FORMAT_R8_UNORM,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST,
        .level_count = 1,
        .layer_count = 1,
        .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
    });
}

render_id distance_field_manager::add_distance_field(const distance_field& distance_field_data)
{
    render_id distance_field_id = m_distance_fields.add();

    auto& distance_field = m_distance_fields[distance_field_id];

    std::uint32_t brick_count = distance_field_data.get_brick_count();
    distance_field.brick_table = m_brick_table->allocate(brick_count);

    distance_field.blocks.resize(
        (distance_field_data.get_active_brick_count() + block_size - 1) / block_size);
    for (std::uint32_t& block : distance_field.blocks)
    {
        block = m_block_allocator.allocate();
    }

    distance_field_upload_request upload_request;
    upload_request.brick_data.resize(distance_field_data.brick_data.size());

    std::uint32_t active_brick_index = 0;
    upload_request.brick_table.resize(brick_count);
    for (std::uint32_t i = 0; i < brick_count; ++i)
    {
        if (distance_field_data.brick_table[i] == SDF_INVALID_BRICK)
        {
            upload_request.brick_table[i] = SDF_INVALID_BRICK;
            continue;
        }

        std::uint32_t block_index = active_brick_index / block_size;
        std::uint32_t block_offset = active_brick_index % block_size;

        upload_request.brick_table[i] =
            distance_field.blocks[block_index] * block_size + block_offset;

        std::size_t brick_data_size = SDF_BRICK_VOXEL_COUNT * sizeof(std::uint8_t);
        std::memcpy(
            upload_request.brick_data.data() + (active_brick_index * brick_data_size),
            distance_field_data.brick_data.data() +
                (distance_field_data.brick_table[i] * brick_data_size),
            brick_data_size);
    }

    return distance_field_id;
}

void distance_field_manager::remove_distance_field(render_id distance_field_id)
{
    auto& distance_field = m_distance_fields[distance_field_id];

    for (std::uint32_t block : distance_field.blocks)
    {
        m_block_allocator.free(block);
    }

    m_distance_fields.remove(distance_field_id);
}

void distance_field_manager::update(gpu_buffer_uploader* uploader)
{
    for (const auto& request : m_upload_queue)
    {
        const auto& distance_field = m_distance_fields[request.distance_field_id];

        auto brick_count =
            static_cast<std::uint32_t>(request.brick_data.size() / SDF_BRICK_VOXEL_COUNT);

        for (std::uint32_t i = 0; i < brick_count; ++i)
        {
            std::uint32_t block_index = distance_field.blocks[i / block_size];
            std::uint32_t block_offset = i % block_size;

            vec3u voxel_coord = get_block_coord(block_index) *
                                vec3u(block_size * SDF_BRICK_SIZE, SDF_BRICK_SIZE, SDF_BRICK_SIZE);
            voxel_coord += vec3u(block_offset * SDF_BRICK_SIZE, 0, 0);

            rhi_texture_region region = {
                .offset_x = static_cast<std::int32_t>(voxel_coord.x),
                .offset_y = static_cast<std::int32_t>(voxel_coord.y),
                .offset_z = static_cast<std::int32_t>(voxel_coord.z),
                .extent =
                    {
                        .width = SDF_BRICK_SIZE,
                        .height = SDF_BRICK_SIZE,
                        .depth = SDF_BRICK_SIZE,
                    },
                .level = 0,
                .layer = 0,
                .layer_count = 1,
                .aspect = RHI_TEXTURE_ASPECT_COLOR,
            };

            uploader->upload(
                m_brick_atlas.get(),
                request.brick_data.data() + static_cast<std::size_t>(i * SDF_BRICK_VOXEL_COUNT),
                SDF_BRICK_VOXEL_COUNT * sizeof(std::uint8_t),
                region,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ,
                RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
        }
    }

    m_distance_fields.update(
        [&](const gpu_distance_field& distance_field) -> gpu_distance_field::gpu_type
        {
            return {};
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader->upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}
} // namespace violet