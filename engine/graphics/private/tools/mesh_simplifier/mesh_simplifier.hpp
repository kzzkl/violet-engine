#pragma once

#include "algorithm/hash.hpp"
#include "math/types.hpp"
#include "tools/mesh_simplifier/collapse_heap.hpp"
#include "tools/mesh_simplifier/quadric.hpp"
#include <functional>
#include <limits>
#include <unordered_map>

namespace violet
{
class mesh_simplifier
{
public:
    void set_positions(std::span<vec3f> positions) noexcept
    {
        m_positions = positions;
    }

    void set_indexes(std::span<std::uint32_t> indexes) noexcept
    {
        m_indexes = indexes;
    }

    template <typename Functor>
    void set_attributes(
        std::span<float> attributes,
        std::span<const float> attribute_weights,
        Functor&& correct_attributes)
    {
        for (float attribute : attributes)
        {
            assert(!std::isnan(attribute));
        }
        m_attributes = attributes;
        m_attribute_weights = attribute_weights;

        assert(m_positions.size() * get_attribute_count() == attributes.size());

        m_correct_attributes = std::forward<Functor>(correct_attributes);
    }

    void lock_position(const vec3f& position)
    {
        m_locked_positions.push_back(position);
    }

    float simplify(
        std::uint32_t target_triangle_count,
        float target_error = std::numeric_limits<float>::max());

    std::uint32_t get_vertex_count() const noexcept
    {
        return m_vertex_count;
    }

    std::uint32_t get_index_count() const noexcept
    {
        return m_index_count;
    }

    const std::vector<vec3f>& get_edge_vertices() const noexcept
    {
        return m_edge_vertices;
    }

private:
    struct vertex_hash
    {
        std::uint32_t operator()(const vec3f& v) const noexcept
        {
            union
            {
                float f;
                std::uint32_t i;
            } x, y, z;

            x.f = v.x;
            y.f = v.y;
            z.f = v.z;

            return hash::murmur_32({x.i, y.i, z.i});
        }
    };

    struct edge
    {
        vec3f p0;
        vec3f p1;

        vec3f merge_position;

        bool removed;

        bool operator==(const edge& other) const noexcept
        {
            return p0 == other.p0 && p1 == other.p1;
        }
    };

    enum corner_flag : std::uint8_t
    {
        CORNER_LOCKED = 1 << 0,
        CORNER_REMOVED = 1 << 1,
        CORNER_EDGE = 1 << 2,
    };
    using corner_flags = std::uint8_t;

    float evaluate_edge(std::uint32_t edge_index);
    void collapse_edge(std::uint32_t edge_index);

    void compact();

    bool add_edge(edge& edge, std::uint32_t index);
    void remove_edge(std::uint32_t index);

    bool get_adjacent_triangles(
        const vec3f& position,
        std::vector<std::uint32_t>& adjacent_triangles);

    quadric_pool get_wedge_quadrics(
        const vec3f& p0,
        const vec3f& p1,
        const std::vector<std::uint32_t>& triangles,
        std::uint32_t* triangle_to_wedge = nullptr);

    void update_edge_quadric(std::uint32_t corner);

    float* get_attributes(std::uint32_t index)
    {
        return m_attributes.data() + static_cast<std::size_t>(index * get_attribute_count());
    }

    std::uint32_t get_attribute_count() const noexcept
    {
        return static_cast<std::uint32_t>(m_attribute_weights.size());
    }

    bool is_triangle_flipped(
        const vec3f& old_position,
        const vec3f& new_position,
        std::uint32_t index) const;
    bool is_triangle_degenerate(const vec3f& p0, const vec3f& p1, const vec3f& p2) const;

    std::span<vec3f> m_positions;
    std::span<std::uint32_t> m_indexes;

    std::span<float> m_attributes;
    std::span<const float> m_attribute_weights;

    std::vector<float> m_wedge_attributes;

    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_vertex_map;
    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_corner_map;
    std::vector<corner_flags> m_corner_flags;
    std::vector<quadric_edge> m_edge_quadrics;

    std::vector<edge> m_edges;
    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_edge_map0;
    std::unordered_multimap<vec3f, std::uint32_t, vertex_hash> m_edge_map1;

    quadric_pool m_triangle_quadrics;
    std::uint32_t m_triangle_count{0};

    std::function<void(float*)> m_correct_attributes;

    std::uint32_t m_vertex_count{0};
    std::uint32_t m_index_count{0};

    std::vector<vec3f> m_edge_vertices;

    std::vector<vec3f> m_locked_positions;

    collapse_heap m_heap;
};

class mesh_simplifier_meshopt
{
public:
    void set_positions(std::span<const vec3f> positions);
    void set_indexes(std::span<const std::uint32_t> indexes);

    float simplify(std::uint32_t target_triangle_count, bool lock_border);

    const std::vector<vec3f>& get_positions() const noexcept
    {
        return m_positions;
    }

    const std::vector<std::uint32_t>& get_indexes() const noexcept
    {
        return m_indexes;
    }

private:
    std::vector<vec3f> m_positions;
    std::vector<std::uint32_t> m_indexes;
};
} // namespace violet