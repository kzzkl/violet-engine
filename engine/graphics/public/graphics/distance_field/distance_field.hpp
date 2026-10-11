#pragma once

#include "graphics/distance_field./sdf_common.hpp"
#include "math/box.hpp"
#include <vector>

namespace violet
{
struct distance_field
{
    vec3u brick_count;
    box3f volume_bounds;

    std::vector<std::uint32_t> brick_table;
    std::vector<std::uint8_t> brick_data;

    std::uint32_t get_brick_count() const noexcept
    {
        return static_cast<std::uint32_t>(brick_table.size());
    }

    std::uint32_t get_active_brick_count() const noexcept
    {
        return static_cast<std::uint32_t>(brick_data.size() / SDF_BRICK_VOXEL_COUNT);
    }
};
} // namespace violet