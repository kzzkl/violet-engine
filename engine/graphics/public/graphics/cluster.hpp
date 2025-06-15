#pragma once

#include "math/box.hpp"
#include "math/sphere.hpp"

namespace violet
{
struct cluster
{
    std::uint32_t index_offset;
    std::uint32_t index_count;

    box3f bounding_box;
    sphere3f bounding_sphere;

    sphere3f lod_bounds;
    float lod_error;

    sphere3f parent_lod_bounds;
    float parent_lod_error;

    std::uint32_t lod;
};

struct cluster_node
{
    sphere3f bounding_sphere;

    sphere3f lod_bounds;
    float min_lod_error;
    float max_parent_lod_error;

    bool is_leaf;
    std::uint32_t depth;
    std::uint32_t child_offset;
    std::uint32_t child_count;
};
} // namespace violet