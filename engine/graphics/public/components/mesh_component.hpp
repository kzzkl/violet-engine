#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include <vector>

namespace violet
{
struct mesh_component
{
    struct submesh
    {
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;
        material* material;
    };

    geometry* geometry{nullptr};
    std::vector<submesh> submeshes;
};
} // namespace violet