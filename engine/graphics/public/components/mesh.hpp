#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include <vector>

namespace violet
{
struct mesh
{
    struct submesh
    {
        std::uint32_t vertex_start;
        std::uint32_t vertex_count;
        std::uint32_t index_start;
        std::uint32_t index_count;
        material* material;
    };

    geometry* geometry{nullptr};
    std::vector<submesh> submeshes;
};

struct mesh_render_data
{
    rhi_ptr<rhi_parameter> parameter;
    std::size_t version{std::numeric_limits<std::size_t>::max()};
};
} // namespace violet