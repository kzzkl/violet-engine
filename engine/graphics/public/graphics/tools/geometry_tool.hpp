#pragma once

#include "math/types.hpp"
#include <span>
#include <vector>

namespace violet
{
class geometry_tool
{
public:
    static std::vector<vec4f> generate_tangents(
        std::span<const vec3f> positions,
        std::span<const vec3f> normals,
        std::span<const vec2f> texcoords,
        std::span<const std::uint32_t> indexes);

    static std::vector<vec3f> generate_smooth_normals(
        std::span<const vec3f> positions,
        std::span<const vec3f> normals,
        std::span<const vec4f> tangents,
        std::span<const std::uint32_t> indexes);

    struct cluster
    {
        std::uint32_t index_offset;
        std::uint32_t index_count;

        std::uint32_t group_index;
    };

    struct cluster_group
    {
    };

    struct cluster_result
    {
        std::vector<std::uint32_t> indexes;
        std::vector<cluster> clusters;
        std::vector<cluster_group> groups;
    };

    static cluster_result generate_clusters(
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes);

    struct simplify_result
    {
        std::vector<vec3f> positions;
        std::vector<std::uint32_t> indexes;
    };

    static simplify_result simplify(
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes,
        std::size_t target_triangle_count,
        std::span<const vec3f> locked_positions = {});
};
} // namespace violet