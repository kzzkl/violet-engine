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
        std::uint32_t index;
        material* material;
    };

    geometry* geometry{nullptr};
    std::vector<submesh> submeshes;
};
} // namespace violet