#include "graphics/tools/cluster_tool.hpp"
#include "algorithm/disjoint_set.hpp"
#include "algorithm/radix_sort.hpp"
#include "math/box.hpp"
#include "math/vector.hpp"
#include "metis.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <numeric>
#include <unordered_map>

namespace violet
{
namespace
{
constexpr std::size_t MAX_CLUSTER_ELEMENT_SIZE = 128;

std::uint32_t murmur_mix_32(std::uint32_t hash)
{
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;

    return hash;
}

std::uint32_t murmur_32(std::initializer_list<std::uint32_t> data)
{
    std::uint32_t hash = 0;
    for (std::uint32_t element : data)
    {
        element *= 0xcc9e2d51;
        element = (element << 15) | (element >> (32 - 15));
        element *= 0x1b873593;

        hash ^= element;
        hash = (hash << 13) | (hash >> (32 - 13));
        hash = hash * 5 + 0xe6546b64;
    }

    return murmur_mix_32(hash);
}

std::uint32_t morton_code_3(std::uint32_t x)
{
    x &= 0x000003ff;
    x = (x ^ (x << 16)) & 0xff0000ff;
    x = (x ^ (x << 8)) & 0x0300f00f;
    x = (x ^ (x << 4)) & 0x030c30c3;
    x = (x ^ (x << 2)) & 0x09249249;
    return x;
}

std::uint32_t hash_position(const vec3f& position)
{
    union
    {
        float f;
        std::uint32_t i;
    } x, y, z;

    x.f = position.x;
    y.f = position.y;
    z.f = position.z;

    return murmur_32({x.i, y.i, z.i});
}

std::uint32_t cycle3(std::uint32_t value)
{
    std::uint32_t value_mod3 = value % 3;
    std::uint32_t value1_mod3 = (1 << value_mod3) & 3;
    return value - value_mod3 + value1_mod3;
}

class graph_linker
{
public:
    void build_external_links(
        disjoint_set<std::uint32_t>& disjoint_set,
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes)
    {
        std::size_t element_count = indexes.size() / 3;

        auto get_center = [&](std::uint32_t triangle_index) -> vec3f
        {
            vec3f center = positions[indexes[triangle_index * 3 + 0]];
            center += positions[indexes[triangle_index * 3 + 1]];
            center += positions[indexes[triangle_index * 3 + 2]];
            center /= 3.0f;

            return center;
        };

        std::vector<std::uint32_t> sorted_to_index(element_count);
        {
            box3f bounds;
            for (const vec3f& position : positions)
            {
                box::expand(bounds, position);
            }

            std::vector<std::uint32_t> sort_keys(element_count);
            for (std::size_t i = 0; i < element_count; ++i)
            {
                vec3f center = (get_center(i) - bounds.min) / vector::max(bounds.max - bounds.min);

                std::uint32_t key = morton_code_3(static_cast<std::uint32_t>(center.x * 1023.0f));
                key |= morton_code_3(static_cast<std::uint32_t>(center.y * 1023.0f)) << 1;
                key |= morton_code_3(static_cast<std::uint32_t>(center.z * 1023.0f)) << 2;
                sort_keys[i] = key;
            }

            std::iota(sorted_to_index.begin(), sorted_to_index.end(), 0);
            radix_sort_32(
                sorted_to_index.begin(),
                sorted_to_index.end(),
                [&](std::uint32_t i)
                {
                    return sort_keys[i];
                });
        }

        std::vector<std::pair<std::size_t, std::size_t>> island_ranges(element_count);
        {
            std::size_t island_index = 0;
            std::size_t first_element = 0;
            for (std::size_t i = 0; i < element_count; ++i)
            {
                std::uint32_t current_island_index = disjoint_set.find(sorted_to_index[i]);
                if (current_island_index != island_index)
                {
                    for (std::size_t j = first_element; j < i; ++j)
                    {
                        island_ranges[j] = {first_element, i};
                    }

                    first_element = i;
                    island_index = current_island_index;
                }
            }

            for (std::size_t i = first_element; i < element_count; ++i)
            {
                island_ranges[i] = {first_element, element_count};
            }
        }

        for (std::size_t i = 0; i < element_count; ++i)
        {
            if (island_ranges[i].second - island_ranges[i].first > MAX_CLUSTER_ELEMENT_SIZE)
            {
                continue;
            }

            std::uint32_t element_index = sorted_to_index[i];
            std::size_t island_index = disjoint_set.find(element_index);

            vec3f center = get_center(element_index);

            static constexpr std::size_t max_link_count = 5;

            std::array<std::uint32_t, max_link_count> cloest_elements;
            cloest_elements.fill(~0u);

            std::array<float, max_link_count> closest_distances;
            closest_distances.fill(std::numeric_limits<float>::max());

            for (int step = -1; step <= 1; step += 2)
            {
                std::uint32_t limit = step == -1 ? 0 : element_count - 1;
                std::uint32_t adjacency = i;

                for (std::size_t iterations = 0; iterations < 16; ++iterations)
                {
                    if (adjacency == limit)
                    {
                        break;
                    }

                    adjacency += step;

                    std::uint32_t adjacency_index = sorted_to_index[adjacency];
                    std::size_t adjacency_island_index = disjoint_set.find(adjacency_index);

                    if (adjacency_island_index == island_index)
                    {
                        adjacency = step == -1 ? island_ranges[adjacency].first :
                                                 island_ranges[adjacency].second - 1;
                    }
                    else
                    {
                        float distance = vector::length_sq(center - get_center(adjacency_index));
                        for (std::size_t link = 0; link < closest_distances.size(); ++link)
                        {
                            if (distance < closest_distances[link])
                            {
                                std::swap(distance, closest_distances[link]);
                                std::swap(adjacency_index, cloest_elements[link]);
                            }
                        }
                    }
                }
            }

            for (std::uint32_t cloest_element : cloest_elements)
            {
                if (cloest_element != ~0u)
                {
                    m_external_links.insert({element_index, cloest_element});
                    m_external_links.insert({cloest_element, element_index});
                }
            }
        }
    }

    void get_external_link(
        std::uint32_t element_index,
        std::vector<std::uint32_t>& adjacency,
        std::vector<std::uint32_t>& adjacency_cost)
    {
        auto range = m_external_links.equal_range(element_index);
        for (auto iter = range.first; iter != range.second; ++iter)
        {
            adjacency.push_back(iter->second);
            adjacency_cost.push_back(1);
        }
    }

private:
    std::unordered_multimap<std::uint32_t, std::uint32_t> m_external_links;
};

class graph_partitioner
{
public:
    void partition(
        std::span<const std::uint32_t> adjacency,
        std::span<const std::uint32_t> adjacency_cost,
        std::span<const std::uint32_t> adjacency_offset)
    {
        std::size_t vertex_count = adjacency_offset.size() - 1;

        m_vertices.resize(vertex_count);
        std::iota(m_vertices.begin(), m_vertices.end(), 0);

        if (vertex_count <= MAX_CLUSTER_ELEMENT_SIZE)
        {
            m_parts.emplace_back(0, static_cast<std::uint32_t>(vertex_count));
            return;
        }

        m_vertex_map.resize(vertex_count);
        std::iota(m_vertex_map.begin(), m_vertex_map.end(), 0);

        m_adjacency.assign(adjacency.begin(), adjacency.end());
        m_adjacency_cost.assign(adjacency_cost.begin(), adjacency_cost.end());
        m_adjacency_offset.assign(adjacency_offset.begin(), adjacency_offset.end());

        partition_recursive(
            0,
            m_vertices.size(),
            MAX_CLUSTER_ELEMENT_SIZE - 4,
            MAX_CLUSTER_ELEMENT_SIZE);
    }

    std::span<const std::uint32_t> get_vertices() const
    {
        return m_vertices;
    }

    std::span<const std::pair<std::size_t, std::size_t>> get_parts() const
    {
        return m_parts;
    }

private:
    std::size_t bisect_graph(
        std::size_t start,
        std::size_t end,
        std::size_t min_count,
        std::size_t max_count)
    {
        std::vector<idx_t> adjacency;
        std::vector<idx_t> adjacency_cost;
        std::vector<idx_t> adjacency_offset;

        for (std::size_t i = start; i < end; ++i)
        {
            adjacency_offset.push_back(static_cast<idx_t>(adjacency.size()));

            for (std::size_t j = m_adjacency_offset[m_vertices[i]];
                 j < m_adjacency_offset[m_vertices[i] + 1];
                 ++j)
            {
                std::uint32_t adjacency_index = m_vertex_map[m_adjacency[j]];
                if (adjacency_index >= start && adjacency_index < end)
                {
                    adjacency.push_back(static_cast<idx_t>(adjacency_index - start));
                    adjacency_cost.push_back(m_adjacency_cost[j]);
                }
            }
        }
        adjacency_offset.push_back(static_cast<idx_t>(adjacency.size()));

        idx_t options[METIS_NOPTIONS];
        METIS_SetDefaultOptions(options);
        options[METIS_OPTION_UFACTOR] = 200;

        auto vertex_count = static_cast<idx_t>(adjacency_offset.size() - 1);
        idx_t part_count = 2;
        idx_t constraints_count = 1;
        idx_t edges_cut = 0;

        auto target_part_count = static_cast<std::size_t>(std::roundf(
            static_cast<float>(end - start) / (static_cast<float>(min_count + max_count) / 2.0f)));
        target_part_count = std::max(2ull, target_part_count);

        std::size_t left_part_count = target_part_count / 2;

        real_t weights[2] = {
            static_cast<float>(left_part_count) / static_cast<float>(target_part_count),
            1.0f - static_cast<float>(left_part_count) / static_cast<float>(target_part_count),
        };

        std::vector<idx_t> parts(vertex_count);

        int result = METIS_PartGraphRecursive(
            &vertex_count,
            &constraints_count,
            adjacency_offset.data(),
            adjacency.data(),
            nullptr,
            nullptr,
            adjacency_cost.data(),
            &part_count,
            weights,
            nullptr,
            options,
            &edges_cut,
            parts.data());
        assert(result == METIS_OK);

        std::size_t left = 0;
        std::size_t right = end - start - 1;

        while (left < right)
        {
            while (parts[left] == 0)
            {
                ++left;
            }

            while (parts[right] == 1)
            {
                --right;
            }

            if (left < right)
            {
                std::swap(parts[left], parts[right]);
                std::swap(m_vertices[start + left], m_vertices[start + right]);
                m_vertex_map[m_vertices[start + left]] = start + left;
                m_vertex_map[m_vertices[start + right]] = start + right;
            }
        }

        return start + left;
    }

    void partition_recursive(
        std::size_t start,
        std::size_t end,
        std::size_t min_count,
        std::size_t max_count)
    {
        std::size_t split = bisect_graph(start, end, min_count, max_count);

        if (split - start > max_count)
        {
            partition_recursive(start, split, min_count, max_count);
        }
        else
        {
            m_parts.emplace_back(start, split);
        }

        if (end - split > max_count)
        {
            partition_recursive(split, end, min_count, max_count);
        }
        else
        {
            m_parts.emplace_back(split, end);
        }
    }

    std::vector<std::uint32_t> m_vertices;
    std::vector<std::size_t> m_vertex_map;

    std::vector<idx_t> m_adjacency;
    std::vector<idx_t> m_adjacency_cost;
    std::vector<idx_t> m_adjacency_offset;

    std::vector<std::pair<std::size_t, std::size_t>> m_parts;
};
} // namespace

cluster_result cluster_tool::generate_clusters(
    std::span<const vec3f> positions,
    std::span<const std::uint32_t> indexes)
{
    std::size_t edge_count = indexes.size();
    std::size_t triangle_count = edge_count / 3;

    disjoint_set<std::uint32_t> disjoint_set(triangle_count);

    std::unordered_multimap<std::uint32_t, std::uint32_t> edge_map;
    for (std::uint32_t edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        std::uint32_t p0_hash = hash_position(positions[indexes[edge_index]]);
        std::uint32_t p1_hash = hash_position(positions[indexes[cycle3(edge_index)]]);
        std::uint32_t edge_hash = murmur_32({p0_hash, p1_hash});

        edge_map.insert({edge_hash, edge_index});
    }

    std::vector<std::uint32_t> edge_adjacency;
    std::vector<std::uint32_t> edge_adjacency_offset;
    edge_adjacency_offset.reserve(edge_count + 1);
    for (std::uint32_t edge_index = 0; edge_index < edge_count; ++edge_index)
    {
        edge_adjacency_offset.push_back(static_cast<std::uint32_t>(edge_adjacency.size()));

        std::uint32_t p0_hash = hash_position(positions[indexes[edge_index]]);
        std::uint32_t p1_hash = hash_position(positions[indexes[cycle3(edge_index)]]);
        std::uint32_t edge_hash = murmur_32({p1_hash, p0_hash});

        auto range = edge_map.equal_range(edge_hash);
        for (auto iter = range.first; iter != range.second; ++iter)
        {
            if (positions[indexes[iter->second]] != positions[indexes[cycle3(edge_index)]] ||
                positions[indexes[cycle3(iter->second)]] != positions[indexes[edge_index]])
            {
                continue;
            }

            edge_adjacency.push_back(iter->second);
            disjoint_set.merge(edge_index / 3, iter->second / 3);
        }
    }
    edge_adjacency_offset.push_back(static_cast<std::uint32_t>(edge_adjacency.size()));

    graph_linker linker;
    linker.build_external_links(disjoint_set, positions, indexes);

    std::vector<std::uint32_t> triangle_adjacency;
    std::vector<std::uint32_t> triangle_adjacency_cost;
    std::vector<std::uint32_t> triangle_adjacency_offset;
    triangle_adjacency_offset.reserve(triangle_count + 1);
    for (std::uint32_t triangle_index = 0; triangle_index < triangle_count; ++triangle_index)
    {
        triangle_adjacency_offset.push_back(static_cast<std::uint32_t>(triangle_adjacency.size()));

        for (std::uint32_t i = 0; i < 3; ++i)
        {
            std::uint32_t edge_index = triangle_index * 3 + i;

            for (std::size_t j = edge_adjacency_offset[edge_index];
                 j < edge_adjacency_offset[edge_index + 1];
                 ++j)
            {
                std::uint32_t adjacency_edge_index = edge_adjacency[j];
                std::uint32_t adjacency_triangle_index = adjacency_edge_index / 3;

                triangle_adjacency.push_back(adjacency_triangle_index);
                triangle_adjacency_cost.push_back(4 * 65);
            }
        }

        linker.get_external_link(triangle_index, triangle_adjacency, triangle_adjacency_cost);
    }
    triangle_adjacency_offset.push_back(static_cast<std::uint32_t>(triangle_adjacency.size()));

    graph_partitioner partitioner;
    partitioner.partition(triangle_adjacency, triangle_adjacency_cost, triangle_adjacency_offset);

    auto triangles = partitioner.get_vertices();
    auto parts = partitioner.get_parts();

    cluster_result result;

    result.indexes.reserve(indexes.size());
    for (std::uint32_t triangle_index : triangles)
    {
        result.indexes.push_back(indexes[triangle_index * 3 + 0]);
        result.indexes.push_back(indexes[triangle_index * 3 + 1]);
        result.indexes.push_back(indexes[triangle_index * 3 + 2]);
    }

    result.clusters.reserve(parts.size());
    for (auto [start, end] : parts)
    {
        result.clusters.push_back({
            .index_offset = static_cast<std::uint32_t>(start * 3),
            .index_count = static_cast<std::uint32_t>((end - start) * 3),
        });
    }

    return result;
}
} // namespace violet