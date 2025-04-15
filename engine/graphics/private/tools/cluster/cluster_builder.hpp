#pragma once

#include "math/box.hpp"
#include "math/types.hpp"
#include <span>
#include <vector>

namespace violet
{
struct cluster
{
    std::uint32_t index_offset;
    std::uint32_t index_count;

    std::vector<std::uint8_t> external_edges;

    box3f bounds;
};

struct cluster_group
{
    std::vector<std::uint32_t> clusters;
};

class cluster_builder
{
public:
    void build(std::span<const vec3f> positions, std::span<const std::uint32_t> indexes);

    const std::vector<std::uint32_t>& get_indexes() const noexcept
    {
        return m_indexes;
    }

    const std::vector<cluster>& get_clusters() const noexcept
    {
        return m_clusters;
    }

    const std::vector<cluster_group>& get_groups() const noexcept
    {
        return m_groups;
    }

private:
    void cluster_triangles(
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes);
    void build_cluster_groups(std::span<const vec3f> positions);
    void simplify_cluster_group(cluster_group& group);

    box3f m_bounds;

    std::vector<std::uint32_t> m_indexes;
    std::vector<cluster> m_clusters;
    std::vector<cluster_group> m_groups;

    struct lod
    {
        std::vector<vec3f> positions;
        std::vector<std::uint32_t> indexes;
    };

    std::vector<lod> m_lods;
};
} // namespace violet