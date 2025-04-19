#pragma once

#include "math/box.hpp"
#include "math/types.hpp"
#include <span>
#include <vector>

namespace violet
{
class cluster_builder
{
public:
    struct cluster
    {
        enum external_edge_flag : std::uint8_t
        {
            EXTERNAL_EDGE_CLUSTER = 1 << 0,
            EXTERNAL_EDGE_GROUP = 1 << 1,
        };
        using external_edge_flags = std::uint8_t;

        std::uint32_t index_offset;
        std::uint32_t index_count;

        std::vector<external_edge_flags> external_edges;

        box3f bounds;
    };

    struct cluster_group
    {
        std::uint32_t cluster_offset;
        std::uint32_t cluster_count;
    };

    struct mesh_lod
    {
        std::vector<vec3f> positions;
        std::vector<std::uint32_t> indexes;

        std::vector<cluster> clusters;
        std::vector<cluster_group> groups;
    };

    std::vector<mesh_lod> build(
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes);

private:
    void cluster_triangles(mesh_lod& lod);
    void group_clusters(mesh_lod& lod);
    void simplify_group(const mesh_lod& lod, mesh_lod& next_lod);

    box3f m_bounds;
};
} // namespace violet