#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include <vector>

namespace violet
{
enum mesh_flag
{
    MESH_NONE = 0,
    MESH_STATIC = 1 << 0,
};
using mesh_flags = std::uint32_t;

struct mesh_component
{
    struct submesh
    {
        std::uint32_t index;
        material* material;
    };

    geometry* geometry{nullptr};
    std::vector<submesh> submeshes;

    mesh_flags flags{MESH_NONE};

    bool visible{true};
};
} // namespace violet