#pragma once

#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "math/box.hpp"
#include "math/sphere.hpp"
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

    box3f bounding_box;
    sphere3f bounding_sphere;
};
} // namespace violet