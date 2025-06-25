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

    struct cluster_node
    {
        box3f bounding_box;
        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float min_lod_error;
        float max_parent_lod_error;

        bool is_leaf;
        std::vector<std::uint32_t> children;
        std::uint32_t depth;
    };

    void set_positions(std::span<const vec3f> positions);
    void set_normals(std::span<const vec3f> normals);
    void set_tangents(std::span<const vec4f> tangents);
    void set_texcoords(std::span<const vec2f> texcoords);
    void set_indexes(std::span<const std::uint32_t> indexes);

    void build();

    const std::vector<cluster>& get_clusters() const noexcept
    {
        return m_clusters;
    }

    const std::vector<cluster_node>& get_cluster_nodes() const noexcept
    {
        return m_cluster_nodes;
    }

    const std::vector<cluster_group>& get_groups() const noexcept
    {
        return m_groups;
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

    std::uint32_t build_bvh(std::span<std::uint32_t> indexes, bool root);
    void sort_groups(std::span<std::uint32_t> group_indexes, std::uint32_t split);
    void calculate_bvh_error();
    void calculate_bvh_depth();

    std::uint32_t get_attribute_count() const noexcept
    {
        std::uint32_t attribute_count = 0;

        attribute_count += m_normals.empty() ? 0 : 3;
        attribute_count += m_tangents.empty() ? 0 : 4;
        attribute_count += m_texcoords.empty() ? 0 : 2;

        return attribute_count;
    }

    box3f m_bounds;

    std::vector<cluster> m_clusters;
    std::vector<cluster_node> m_cluster_nodes;
    std::vector<cluster_group> m_groups;

    std::vector<vec3f> m_positions;
    std::vector<vec3f> m_normals;
    std::vector<vec4f> m_tangents;
    std::vector<vec2f> m_texcoords;
    std::vector<std::uint32_t> m_indexes;
};
} // namespace violet