#pragma once

#include "graphics/cluster.hpp"
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

    struct simplify_input
    {
        std::span<vec3f> positions;
        std::span<vec3f> normals;
        std::span<vec4f> tangents;
        std::span<vec2f> texcoords;
        std::span<std::uint32_t> indexes;
        std::span<const vec3f> locked_positions;

        std::uint32_t target_triangle_count;
    };

    struct simplify_output
    {
        std::uint32_t vertex_count;
        std::uint32_t index_count;
        std::vector<vec3f> edge_vertices;
    };

    static simplify_output simplify(const simplify_input& input);

    struct cluster_input
    {
        struct submesh
        {
            std::uint32_t vertex_offset;
            std::uint32_t index_offset;
            std::uint32_t index_count;
        };

        std::span<const vec3f> positions;
        std::span<const std::uint32_t> indexes;
        std::vector<submesh> submeshes;
    };

    struct cluster_output
    {
        struct submesh
        {
            std::vector<cluster> clusters;
            std::vector<cluster_node> cluster_nodes;
        };

        std::vector<vec3f> positions;
        std::vector<std::uint32_t> indexes;
        std::vector<submesh> submeshes;

        bool load(std::string_view path);
        bool save(std::string_view path) const;
    };

    static cluster_output generate_clusters(const cluster_input& input);
};
} // namespace violet