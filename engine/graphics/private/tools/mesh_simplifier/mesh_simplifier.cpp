#include "tools/mesh_simplifier/mesh_simplifier.hpp"
#include "algorithm/disjoint_set.hpp"
#include "math/vector.hpp"
#include "meshoptimizer.h"
#include <cassert>
#include <cstddef>

namespace violet
{
namespace
{
std::uint32_t cycle_3(std::uint32_t value)
{
    std::uint32_t value_mod3 = value % 3;
    std::uint32_t value1_mod3 = (1 << value_mod3) & 3;
    return value - value_mod3 + value1_mod3;
}
} // namespace

void mesh_simplifier::set_positions(std::span<const vec3f> positions)
{
    m_vertex_map.clear();

    m_positions.assign(positions.begin(), positions.end());

    for (std::uint32_t i = 0; i < m_positions.size(); ++i)
    {
        m_vertex_map.insert({m_positions[i], i});
    }
}

void mesh_simplifier::set_attributes(
    std::span<const float> attributes,
    std::uint32_t attribute_count)
{
    assert(m_positions.size() * attribute_count == attributes.size());
    m_attributes.assign(attributes.begin(), attributes.end());

    m_attribute_count = attribute_count;
}

void mesh_simplifier::set_indexes(std::span<const std::uint32_t> indexes)
{
    assert(!m_positions.empty());

    m_indexes.assign(indexes.begin(), indexes.end());

    for (std::uint32_t corner = 0; corner < indexes.size(); ++corner)
    {
        vec3f position = m_positions[indexes[corner]];

        m_corner_map.insert({position, corner});

        std::uint32_t other_corner = cycle_3(corner);

        edge edge = {
            .p0 = position,
            .p1 = m_positions[indexes[other_corner]],
        };
        if (add_edge(edge, static_cast<std::uint32_t>(m_edges.size())))
        {
            m_edges.push_back(edge);
        }
    }

    m_corner_flags.resize(indexes.size());
}

void mesh_simplifier::lock_position(const vec3f& position)
{
    auto corner_range = m_corner_map.equal_range(position);
    assert(corner_range.first != corner_range.second);

    for (auto iter = corner_range.first; iter != corner_range.second; ++iter)
    {
        m_corner_flags[iter->second] |= CORNER_LOCKED;
    }
}

float mesh_simplifier::simplify(std::uint32_t target_triangle_count)
{
    m_triangle_count = static_cast<std::uint32_t>(m_indexes.size() / 3);
    m_triangle_quadrics.resize(m_triangle_count, m_attribute_count);

    for (std::uint32_t i = 0; i < m_triangle_count; ++i)
    {
        m_triangle_quadrics[i].set(
            m_positions[m_indexes[3 * i + 0]],
            m_positions[m_indexes[3 * i + 1]],
            m_positions[m_indexes[3 * i + 2]],
            get_attributes(m_indexes[3 * i + 0]),
            get_attributes(m_indexes[3 * i + 1]),
            get_attributes(m_indexes[3 * i + 2]),
            m_attribute_count);
    }

    for (std::uint32_t edge = 0; edge < m_edges.size(); ++edge)
    {
        float error = evaluate_edge(edge);
        m_heap.push({
            .edge = edge,
            .error = error,
        });
    }

    float max_error = 0.0f;
    while (!m_heap.empty() && m_triangle_count > target_triangle_count)
    {
        auto current_edge = m_heap.top();
        m_heap.pop();

        collapse_edge(current_edge.edge);

        max_error = std::max(max_error, current_edge.error);
    }

    compact();

    return std::sqrt(max_error);
}

float mesh_simplifier::evaluate_edge(std::uint32_t edge_index)
{
    vec3f p0 = m_edges[edge_index].p0;
    vec3f p1 = m_edges[edge_index].p1;

    std::vector<std::uint32_t> adjacent_triangles;

    bool p0_locked = get_adjacent_triangles(p0, adjacent_triangles);
    bool p1_locked = get_adjacent_triangles(p1, adjacent_triangles);

    if (adjacent_triangles.empty())
    {
        return 0.0f;
    }

    quadric_pool wedge_quadrics = get_wedge_quadrics(adjacent_triangles);

    quadric_optimizer optimizer = {};
    for (std::size_t i = 0; i < wedge_quadrics.get_size(); ++i)
    {
        optimizer.add(wedge_quadrics[i], m_attribute_count);
    }

    vec3f new_position;
    if (p0_locked && !p1_locked)
    {
        new_position = p0;
    }
    else if (!p0_locked && p1_locked)
    {
        new_position = p1;
    }
    else
    {
        auto optimize_position = optimizer.optimize();

        if (optimize_position)
        {
            new_position = *optimize_position;

            if (vector::length(new_position - p0) + vector::length(new_position - p1) >
                2.0f * vector::length(p0 - p1))
            {
                new_position = (p0 + p1) * 0.5f;
            }
        }
        else
        {
            new_position = (p0 + p1) * 0.5f;
        }
    }

    float error = p0_locked && p1_locked ? 1e+8f : 0.0f;

    for (std::size_t i = 0; i < wedge_quadrics.get_size(); ++i)
    {
        float wedge_error = wedge_quadrics[i].evaluate(new_position, nullptr, m_attribute_count);
        error += wedge_error;
    }

    for (std::uint32_t triangle : adjacent_triangles)
    {
        if (is_triangle_flip(p0, new_position, triangle) ||
            is_triangle_flip(p1, new_position, triangle))
        {
            error += 1e+4f;
        }
    }

    m_edges[edge_index].merge_position = new_position;

    return error;
}

void mesh_simplifier::collapse_edge(std::uint32_t edge_index)
{
    vec3f p0 = m_edges[edge_index].p0;
    vec3f p1 = m_edges[edge_index].p1;
    vec3f merge_position = m_edges[edge_index].merge_position;

    std::vector<std::uint32_t> triangles;
    bool p0_locked = get_adjacent_triangles(p0, triangles);
    bool p1_locked = get_adjacent_triangles(p1, triangles);

    std::sort(triangles.begin(), triangles.end());
    triangles.erase(std::unique(triangles.begin(), triangles.end()), triangles.end());

    std::vector<std::uint32_t> triangle_to_wedge(triangles.size());
    auto wedge_quadrics = get_wedge_quadrics(triangles, triangle_to_wedge.data());

    std::vector<std::uint32_t> moved_vertices;
    std::vector<std::uint32_t> moved_corners;
    std::vector<std::uint32_t> moved_edges;
    std::vector<std::uint32_t> reevaluate_edges;

    for (const auto& position : {p0, p1})
    {
        auto vertex_range = m_vertex_map.equal_range(position);
        for (auto iter = vertex_range.first; iter != vertex_range.second; ++iter)
        {
            moved_vertices.push_back(iter->second);
        }
        m_vertex_map.erase(position);

        auto corner_range = m_corner_map.equal_range(position);
        for (auto iter = corner_range.first; iter != corner_range.second; ++iter)
        {
            moved_corners.push_back(iter->second);
        }
        m_corner_map.erase(position);

        auto edge_range0 = m_edge_map0.equal_range(position);
        for (auto iter = edge_range0.first; iter != edge_range0.second; ++iter)
        {
            moved_edges.push_back(iter->second);
        }

        auto edge_range1 = m_edge_map1.equal_range(position);
        for (auto iter = edge_range1.first; iter != edge_range1.second; ++iter)
        {
            moved_edges.push_back(iter->second);
        }
    }

    for (std::uint32_t triangle_index = 0; triangle_index < triangles.size(); ++triangle_index)
    {
        std::uint32_t triangle = triangles[triangle_index];

        for (std::uint32_t i = 0; i < 3; ++i)
        {
            std::uint32_t corner = triangle * 3 + i;
            vec3f& position = m_positions[m_indexes[corner]];

            if (position == p0 || position == p1)
            {
                position = merge_position;

                auto& wedge_quadric = wedge_quadrics[triangle_to_wedge[triangle_index]];
                wedge_quadric.evaluate(
                    position,
                    get_attributes(m_indexes[corner]),
                    m_attribute_count);

                auto* normal = reinterpret_cast<vec3f*>(get_attributes(m_indexes[corner]));
                *normal = vector::normalize(*normal);

                if (p0_locked || p1_locked)
                {
                    m_corner_flags[corner] |= CORNER_LOCKED;
                }
            }
        }

        const vec3f& p0 = m_positions[m_indexes[triangle * 3 + 0]];
        const vec3f& p1 = m_positions[m_indexes[triangle * 3 + 1]];
        const vec3f& p2 = m_positions[m_indexes[triangle * 3 + 2]];

        if (p0 == p1 || p1 == p2 || p2 == p0)
        {
            --m_triangle_count;

            m_corner_flags[triangle * 3 + 0] |= CORNER_REMOVED;
            m_corner_flags[triangle * 3 + 1] |= CORNER_REMOVED;
            m_corner_flags[triangle * 3 + 2] |= CORNER_REMOVED;
        }
        else
        {
            m_triangle_quadrics[triangle].set(
                p0,
                p1,
                p2,
                get_attributes(m_indexes[triangle * 3 + 0]),
                get_attributes(m_indexes[triangle * 3 + 1]),
                get_attributes(m_indexes[triangle * 3 + 2]),
                m_attribute_count);
        }

        for (const auto& position : {p0, p1, p2})
        {
            auto edge_range0 = m_edge_map0.equal_range(position);
            for (auto iter = edge_range0.first; iter != edge_range0.second; ++iter)
            {
                reevaluate_edges.push_back(iter->second);
            }

            auto edge_range1 = m_edge_map1.equal_range(position);
            for (auto iter = edge_range1.first; iter != edge_range1.second; ++iter)
            {
                reevaluate_edges.push_back(iter->second);
            }
        }
    }

    std::sort(reevaluate_edges.begin(), reevaluate_edges.end());
    reevaluate_edges.erase(
        std::unique(reevaluate_edges.begin(), reevaluate_edges.end()),
        reevaluate_edges.end());

    for (std::uint32_t moved_edge : moved_edges)
    {
        remove_edge(moved_edge);

        auto& edge = m_edges[moved_edge];

        if (edge.p0 == p0 || edge.p0 == p1)
        {
            edge.p0 = merge_position;
        }

        if (edge.p1 == p0 || edge.p1 == p1)
        {
            edge.p1 = merge_position;
        }

        if (edge.p0 != edge.p1)
        {
            add_edge(edge, moved_edge);
        }
    }

    for (std::uint32_t moved_vertex : moved_vertices)
    {
        m_vertex_map.insert({merge_position, moved_vertex});
    }

    for (std::uint32_t moved_corner : moved_corners)
    {
        m_corner_map.insert({merge_position, moved_corner});
    }

    for (std::uint32_t reevaluate_edge : reevaluate_edges)
    {
        auto& edge = m_edges[reevaluate_edge];
        if (edge.p0 != edge.p1 && !edge.removed)
        {
            float error = evaluate_edge(reevaluate_edge);
            m_heap.update(reevaluate_edge, error);
        }
        else
        {
            m_heap.erase(reevaluate_edge);
        }
    }
}

void mesh_simplifier::compact()
{
    std::vector<std::uint32_t> vertex_reference_count(m_positions.size());
    for (std::uint32_t corner = 0; corner < m_indexes.size(); ++corner)
    {
        if ((m_corner_flags[corner] & CORNER_REMOVED) == 0)
        {
            ++vertex_reference_count[m_indexes[corner]];
        }
    }

    std::vector<std::uint32_t> vertex_remap(m_positions.size());
    std::size_t vertex_count = 0;
    for (std::uint32_t i = 0; i < m_positions.size(); ++i)
    {
        if (vertex_reference_count[i] > 0)
        {
            m_positions[vertex_count] = m_positions[i];
            for (std::uint32_t j = 0; j < m_attribute_count; ++j)
            {
                m_attributes[vertex_count * m_attribute_count + j] =
                    m_attributes[i * m_attribute_count + j];
            }

            vertex_remap[i] = static_cast<std::uint32_t>(vertex_count);
            ++vertex_count;
        }
    }

    std::uint32_t index_count = 0;
    for (std::uint32_t corner = 0; corner < m_indexes.size(); ++corner)
    {
        if ((m_corner_flags[corner] & CORNER_REMOVED) == 0)
        {
            m_indexes[index_count] = vertex_remap[m_indexes[corner]];
            ++index_count;
        }
    }

    m_positions.resize(vertex_count);
    m_indexes.resize(index_count);
    m_attributes.resize(vertex_count * m_attribute_count);
}

bool mesh_simplifier::add_edge(edge& edge, std::uint32_t index)
{
    vertex_hash hasher;

    std::uint32_t hash0 = hasher(edge.p0);
    std::uint32_t hash1 = hasher(edge.p1);

    if (hash0 < hash1)
    {
        std::swap(edge.p0, edge.p1);
        std::swap(hash0, hash1);
    }

    auto range = m_edge_map0.equal_range(edge.p0);
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        if (m_edges[iter->second] == edge)
        {
            return false;
        }
    }

    m_edge_map0.insert({edge.p0, index});
    m_edge_map1.insert({edge.p1, index});

    edge.removed = false;

    return true;
}

void mesh_simplifier::remove_edge(std::uint32_t index)
{
    auto& edge = m_edges[index];
    edge.removed = true;

    auto range0 = m_edge_map0.equal_range(edge.p0);
    for (auto iter = range0.first; iter != range0.second; ++iter)
    {
        if (iter->second == index)
        {
            m_edge_map0.erase(iter);
            break;
        }
    }

    auto range1 = m_edge_map1.equal_range(edge.p1);
    for (auto iter = range1.first; iter != range1.second; ++iter)
    {
        if (iter->second == index)
        {
            m_edge_map1.erase(iter);
            break;
        }
    }
}

bool mesh_simplifier::get_adjacent_triangles(
    const vec3f& position,
    std::vector<std::uint32_t>& adjacent_triangles)
{
    bool locked = false;

    auto range = m_corner_map.equal_range(position);
    for (auto iter = range.first; iter != range.second; ++iter)
    {
        std::uint32_t corner = iter->second;
        if (m_corner_flags[corner] & CORNER_REMOVED)
        {
            continue;
        }

        locked = locked || (m_corner_flags[corner] & CORNER_LOCKED);

        std::uint32_t triangle = corner / 3;
        adjacent_triangles.push_back(triangle);
    }

    return locked;
}

quadric_pool mesh_simplifier::get_wedge_quadrics(
    const std::vector<std::uint32_t>& triangles,
    std::uint32_t* triangle_to_wedge)
{
    struct wedge
    {
        std::uint32_t vertex_index;
        std::uint32_t triangle_index;
    };

    std::vector<wedge> wedges;
    disjoint_set<std::uint32_t> wedge_disjoint_set(static_cast<std::uint32_t>(triangles.size()));

    for (std::uint32_t i = 0; i < triangles.size(); ++i)
    {
        for (std::uint32_t j = 0; j < 3; ++j)
        {
            std::uint32_t corner = triangles[i] * 3 + j;
            std::uint32_t vertex_index = m_indexes[corner];

            auto iter = std::find_if(
                wedges.begin(),
                wedges.end(),
                [vertex_index](const wedge& wedge)
                {
                    return wedge.vertex_index == vertex_index;
                });

            if (iter == wedges.end())
            {
                wedges.push_back({
                    .vertex_index = vertex_index,
                    .triangle_index = i,
                });
            }
            else
            {
                wedge_disjoint_set.merge(i, iter->triangle_index);
            }
        }
    }

    quadric_pool wedge_quadrics(wedge_disjoint_set.get_set_count(), m_attribute_count);

    std::vector<std::uint32_t> wedge_index_to_id;
    for (std::uint32_t i = 0; i < triangles.size(); ++i)
    {
        std::uint32_t triangle = triangles[i];

        std::uint32_t wedge_id = wedge_disjoint_set.find(i);
        std::size_t wedge_index;

        auto iter = std::find(wedge_index_to_id.begin(), wedge_index_to_id.end(), wedge_id);
        if (iter == wedge_index_to_id.end())
        {
            wedge_index = wedge_index_to_id.size();
            wedge_index_to_id.push_back(wedge_id);
            wedge_quadrics[wedge_index].set(m_triangle_quadrics[triangle], m_attribute_count);
        }
        else
        {
            wedge_index = iter - wedge_index_to_id.begin();
            wedge_quadrics[wedge_index].add(m_triangle_quadrics[triangle], m_attribute_count);
        }

        if (triangle_to_wedge != nullptr)
        {
            triangle_to_wedge[i] = static_cast<std::uint32_t>(wedge_index);
        }
    }

    return wedge_quadrics;
}

bool mesh_simplifier::is_triangle_flip(
    const vec3f& old_position,
    const vec3f& new_position,
    std::uint32_t index) const
{
    for (std::uint32_t i = 0; i < 3; ++i)
    {
        std::uint32_t corner = index * 3 + i;
        if (m_positions[m_indexes[corner]] == old_position)
        {
            std::uint32_t p1_corner = cycle_3(corner);
            std::uint32_t p2_corner = cycle_3(p1_corner);

            vec3f p1 = m_positions[m_indexes[p1_corner]];
            vec3f p2 = m_positions[m_indexes[p2_corner]];

            vec3f old_normal = vector::cross(p2 - old_position, p1 - old_position);
            vec3f new_normal = vector::cross(p2 - new_position, p1 - new_position);

            return vector::dot(old_normal, new_normal) < 0.0f;
        }
    }

    return false;
}

void mesh_simplifier_meshopt::set_positions(std::span<const vec3f> positions)
{
    m_positions.assign(positions.begin(), positions.end());
}

void mesh_simplifier_meshopt::set_indexes(std::span<const std::uint32_t> indexes)
{
    m_indexes.assign(indexes.begin(), indexes.end());
}

float mesh_simplifier_meshopt::simplify(std::uint32_t target_triangle_count, bool lock_border)
{
    std::vector<std::uint32_t> simplified_indexes(m_indexes.size());

    float error = 0.0f;

    std::size_t simplified_index_count = meshopt_simplify(
        simplified_indexes.data(),
        m_indexes.data(),
        m_indexes.size(),
        &m_positions[0].x,
        m_positions.size(),
        sizeof(vec3f),
        static_cast<size_t>(target_triangle_count) * 3,
        FLT_MAX,
        lock_border ? meshopt_SimplifyLockBorder : 0,
        &error);

    float error_scale = meshopt_simplifyScale(&m_positions[0].x, m_positions.size(), sizeof(vec3f));

    simplified_indexes.resize(simplified_index_count);

    std::swap(m_indexes, simplified_indexes);

    return error * error_scale;
}
} // namespace violet