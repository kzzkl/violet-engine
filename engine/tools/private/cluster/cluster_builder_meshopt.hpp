#pragma once

#include "math/box.hpp"
#include "math/sphere.hpp"
#include <cstdint>
#include <span>
#include <vector>

namespace violet
{
struct cluster_builder_meshopt_options
{
    std::uint32_t max_vertices{128};
    std::uint32_t min_triangles{128 / 3};
    std::uint32_t max_triangles{128};

    bool partition_sort{false};
    bool partition_spatial{true};
    std::uint32_t partition_size{16};

    float simplify_ratio{0.5f};

    bool cluster_spatial{false};
    float cluster_split_factor{2.0f};
    float cluster_fill_weight{0.0f};

    bool optimize_clusters{true};
    int optimize_clusters_level{1};
};

class cluster_builder_meshopt
{
public:
    struct cluster
    {
        std::uint32_t index_offset;
        std::uint32_t index_count;

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

    void build(const cluster_builder_meshopt_options& options = {});

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

    const std::vector<vec3f>& get_normals() const noexcept
    {
        return m_normals;
    }

    const std::vector<vec4f>& get_tangents() const noexcept
    {
        return m_tangents;
    }

    const std::vector<vec2f>& get_texcoords() const noexcept
    {
        return m_texcoords;
    }

    const std::vector<std::uint32_t>& get_indexes() const noexcept
    {
        return m_indexes;
    }

private:
    struct meshopt_cluster
    {
        std::size_t vertex_count;
        std::vector<std::uint32_t> indices;

        box3f bounding_box;
        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float lod_error;

        std::uint32_t refined;
    };

    std::vector<meshopt_cluster> clusterize(
        const float* positions,
        std::size_t position_stride,
        const std::uint32_t* indexes,
        size_t index_count);
    void compute_cluster_bounds(meshopt_cluster& cluster, float lod_error);
    cluster_group merge_cluster_bounds(
        const std::vector<meshopt_cluster>& clusters,
        std::span<const int> group);
    std::vector<std::vector<int>> partition(
        const std::vector<meshopt_cluster>& clusters,
        const std::vector<int>& pending,
        const std::vector<std::uint32_t>& remap);
    void lock_boundaries(
        std::vector<std::uint8_t>& locks,
        const std::vector<std::vector<int>>& groups,
        const std::vector<meshopt_cluster>& clusters,
        const std::vector<std::uint32_t>& remap);
    struct simplify_result
    {
        std::vector<std::uint32_t> indexes;
        float error;
    };

    simplify_result simplify(
        const std::vector<std::uint32_t>& indexes,
        const std::vector<std::uint8_t>& locks,
        std::size_t target_index_count);
    std::uint32_t output_group(
        const std::vector<meshopt_cluster>& clusters,
        std::span<const int> group,
        const cluster_group& simplified,
        std::uint32_t lod);

    std::uint32_t build_bvh(std::span<std::uint32_t> indexes, bool root);
    void sort_groups(std::span<std::uint32_t> group_indexes, std::uint32_t split);
    void calculate_bvh_error();
    void calculate_bvh_depth();

    std::uint32_t get_attribute_count() const
    {
        std::uint32_t count = 0;
        if (!m_normals.empty())
        {
            count += 3;
        }

        if (!m_tangents.empty())
        {
            count += 4;
        }

        if (!m_texcoords.empty())
        {
            count += 2;
        }

        return count;
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

    cluster_builder_meshopt_options m_options;
};
} // namespace violet
