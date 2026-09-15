#pragma once

#include "graphics/distance_field/distance_field.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/resources/persistent_buffer.hpp"

namespace violet
{
class distance_field_manager
{
public:
    distance_field_manager();

    render_id add_distance_field(const distance_field& distance_field_data);
    void remove_distance_field(render_id distance_field_id);

    const box3f& get_volume_bounds(render_id distance_field_id) const
    {
        return m_distance_fields[distance_field_id].volume_bounds;
    }

    void update(gpu_buffer_uploader* uploader);

    rhi_buffer* get_brick_table() const
    {
        return m_brick_table->get_rhi();
    }

    rhi_texture* get_brick_atlas() const
    {
        return m_brick_atlas.get();
    }

private:
    static constexpr std::uint32_t block_size = 16;

    struct gpu_distance_field
    {
        struct gpu_type
        {
            vec4u brick_count;
            std::uint32_t brick_table_offset;
            std::uint32_t brick_data_offset;
            std::uint32_t padding0;
            std::uint32_t padding1;
        };

        vec3u brick_count;
        box3f volume_bounds;

        buffer_allocation brick_table;
        std::vector<std::uint32_t> blocks;
    };

    struct distance_field_upload_request
    {
        render_id distance_field_id;

        std::vector<std::uint32_t> brick_table;
        std::vector<std::uint8_t> brick_data;
    };

    vec3u get_block_coord(std::uint32_t block_index) const
    {
        vec3u coord;
        coord.x = block_index % SDF_BRICK_ATLAS_BLOCK_COUNT_X;
        block_index /= SDF_BRICK_ATLAS_BLOCK_COUNT_X;
        coord.y = block_index % SDF_BRICK_ATLAS_BLOCK_COUNT_Y;
        coord.z = block_index / SDF_BRICK_ATLAS_BLOCK_COUNT_Y;
        return coord;
    }

    gpu_sparse_array<gpu_distance_field> m_distance_fields;

    std::unique_ptr<persistent_buffer> m_brick_table;
    rhi_ptr<rhi_texture> m_brick_atlas;

    index_allocator m_block_allocator;

    std::vector<distance_field_upload_request> m_upload_queue;
};
} // namespace violet