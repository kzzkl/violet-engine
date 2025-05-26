#pragma once

#include "math/box.hpp"
#include "math/sphere.hpp"
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
            EXTERNAL_EDGE_CLUSTER = 1 << 0, // The edge is a external edge of the cluster.
            EXTERNAL_EDGE_GROUP = 1 << 1,   // The edge is a external edge of the group.
        };
        using external_edge_flags = std::uint8_t;

        std::uint32_t index_offset;
        std::uint32_t index_count;

        std::vector<external_edge_flags> external_edges;

        box3f bounding_box;
        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float lod_error;

        std::uint32_t group_index;
        std::uint32_t child_group_index;
    };

    struct cluster_group
    {
        box3f bounding_box;
        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float min_lod_error;
        float max_parent_lod_error;

        std::uint32_t cluster_offset;
        std::uint32_t cluster_count;

        std::uint32_t lod;
    };

    struct cluster_bvh_node
    {
        box3f bounding_box;
        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float min_lod_error;
        float max_parent_lod_error;

        bool is_leaf;
        std::vector<std::uint32_t> children;
    };

    void set_positions(std::span<const vec3f> positions);
    void set_indexes(std::span<const std::uint32_t> indexes);

    void build();

    const std::vector<cluster>& get_clusters() const noexcept
    {
        return m_clusters;
    }

    const std::vector<cluster_group>& get_groups() const noexcept
    {
        return m_groups;
    }

    const std::vector<cluster_bvh_node>& get_bvh_nodes() const noexcept
    {
        return m_bvh_nodes;
    }

    const std::vector<vec3f>& get_positions() const noexcept
    {
        return m_positions;
    }

    const std::vector<std::uint32_t>& get_indexes() const noexcept
    {
        return m_indexes;
    }

private:
    void cluster_triangles(
        const std::vector<vec3f>& positions,
        std::vector<std::uint32_t>& indexes);
    void group_clusters(std::uint32_t cluster_offset, std::uint32_t cluster_count);
    void simplify_group(std::uint32_t group_index);

    void build_bvh(std::uint32_t group_offset, std::uint32_t group_count, std::uint32_t lod);
    void calculate_bvh_error();

    box3f m_bounds;

    std::vector<cluster> m_clusters;
    std::vector<cluster_group> m_groups;

    std::vector<cluster_bvh_node> m_bvh_nodes;

    std::vector<vec3f> m_positions;
    std::vector<std::uint32_t> m_indexes;
};
} // namespace violet