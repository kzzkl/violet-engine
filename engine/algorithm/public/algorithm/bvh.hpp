#pragma once

#include "math/box.hpp"
#include "math/ray.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <optional>
#include <vector>

namespace violet
{
template <typename T>
concept bvh_primitive = requires(const T& p) {
    { p.get_bounds() } -> std::convertible_to<box3f>;
    { p.get_centroid() } -> std::convertible_to<vec3f>;
};

template <typename T>
concept bvh_intersectable = requires(const T& p, const ray3f& ray) {
    { p.intersect(ray) } -> std::same_as<typename ray3f::hit_result>;
};

template <typename T>
concept bvh_distance_computable = requires(const T& p, const vec3f& position) {
    { p.get_distance(position) } -> std::same_as<float>;
};

template <typename T>
concept bvh_distance_sq_computable = requires(const T& p, const vec3f& position) {
    { p.get_distance_sq(position) } -> std::same_as<float>;
};

inline constexpr std::size_t BVH_INVALID_PRIMITIVE_INDEX = std::numeric_limits<std::size_t>::max();

template <typename T>
    requires bvh_primitive<T>
class bvh
{
public:
    using primitive_type = T;

    // The queries keep their traversal stack on the stack frame, so the tree depth needs
    // an upper bound: shallower than max_tree_depth the builder picks the surface area
    // heuristic split, at max_tree_depth and beyond it always picks the balanced median
    // split, which at least halves the primitive count. The depth is therefore bounded by
    // max_tree_depth + log2(primitive count), that is max_tree_depth + 32 for 32 bit
    // counts, and the queries never need more than depth + 1 entries.
    static constexpr std::uint32_t max_tree_depth = 48;
    static constexpr std::size_t traverse_stack_size = max_tree_depth + 40;

    bvh(std::size_t primitive_count = 0)
    {
        reserve(primitive_count);
        m_nodes.emplace_back();
    }

    void add_primitive(const primitive_type& primitive)
    {
        m_primitives.push_back(primitive);
        m_order.push_back(static_cast<std::uint32_t>(m_primitives.size() - 1));

        const box3f& bounds = primitive.get_bounds();

        node& root = m_nodes[0];
        root.bounds_min = vector::min(root.bounds_min, bounds.min);
        root.bounds_max = vector::max(root.bounds_max, bounds.max);
        root.count = static_cast<std::uint32_t>(m_primitives.size());
    }

    void reserve(std::size_t primitive_count)
    {
        m_primitives.reserve(primitive_count);
        m_order.reserve(primitive_count);
        m_nodes.reserve(primitive_count * 2);
    }

    const primitive_type& get_primitive(std::size_t index) const
    {
        assert(index < m_primitives.size());
        return m_primitives[index];
    }

    box3f get_bounds() const noexcept
    {
        box3f result;
        result.min = m_nodes[0].bounds_min;
        result.max = m_nodes[0].bounds_max;
        return result;
    }

    void build(std::uint32_t max_leaf_size = 4)
    {
        m_order.clear();

        if (m_primitives.empty())
        {
            m_nodes.clear();
            m_nodes.emplace_back();
            return;
        }

        assert(m_primitives.size() <= std::numeric_limits<std::uint32_t>::max());

        node root = m_nodes.empty() ? node{} : m_nodes[0];
        root.first = 0;
        root.count = static_cast<std::uint32_t>(m_primitives.size());

        m_nodes.clear();
        m_nodes.push_back(root);

        std::vector<build_entry> entries;
        entries.reserve(m_primitives.size());
        for (std::size_t i = 0; i < m_primitives.size(); ++i)
        {
            entries.push_back({m_primitives[i].get_centroid(), static_cast<std::uint32_t>(i), 0});
        }

        std::vector<std::uint32_t> depths(1, 0);

        std::size_t offset = 0;
        while (offset < m_nodes.size())
        {
            const auto index = static_cast<std::uint32_t>(offset++);
            const std::uint32_t first = m_nodes[index].first;
            const std::uint32_t count = m_nodes[index].count;
            const std::uint32_t depth = depths[index];

            if (count <= max_leaf_size)
            {
                continue;
            }

            split_result result = split(std::span(entries.data() + first, count), depth);

            const auto child = static_cast<std::uint32_t>(m_nodes.size());

            result.left.first += first;
            result.right.first += first;

            m_nodes[index].first = child;
            m_nodes[index].count = 0;

            m_nodes.push_back(result.left);
            m_nodes.push_back(result.right);
            depths.push_back(depth + 1);
            depths.push_back(depth + 1);
        }

        assert(depths.size() == m_nodes.size());

        m_order.resize(entries.size());
        for (std::size_t i = 0; i < entries.size(); ++i)
        {
            m_order[i] = entries[i].index;
        }
    }

    std::vector<std::pair<std::size_t, ray3f::hit_result>> intersect(
        const ray3f& ray,
        float max_distance = std::numeric_limits<float>::infinity()) const
        requires bvh_intersectable<primitive_type>
    {
        std::vector<std::pair<std::size_t, ray3f::hit_result>> result;

        if (m_primitives.empty())
        {
            return result;
        }

        const vec3f inv_direction = vector::div(1.0f, ray.direction);

        std::array<std::uint32_t, traverse_stack_size> stack;
        std::size_t stack_size = 0;

        if (intersect_bounds(m_nodes[0], ray.origin, inv_direction) >= max_distance)
        {
            return result;
        }
        stack[stack_size++] = 0;

        while (stack_size > 0)
        {
            const node& n = m_nodes[stack[--stack_size]];

            if (n.is_leaf())
            {
                for (std::uint32_t i = 0; i < n.count; ++i)
                {
                    const std::uint32_t index = m_order[n.first + i];
                    const auto hit = m_primitives[index].intersect(ray);
                    if (hit && hit.enter < max_distance)
                    {
                        result.push_back({index, hit});
                    }
                }
            }
            else
            {
                if (intersect_bounds(m_nodes[n.first], ray.origin, inv_direction) < max_distance)
                {
                    assert(stack_size < stack.size());
                    stack[stack_size++] = n.first;
                }

                if (intersect_bounds(m_nodes[n.first + 1], ray.origin, inv_direction) <
                    max_distance)
                {
                    assert(stack_size < stack.size());
                    stack[stack_size++] = n.first + 1;
                }
            }
        }

        return result;
    }

    std::optional<std::pair<std::size_t, ray3f::hit_result>> intersect_closest(
        const ray3f& ray,
        float max_distance = std::numeric_limits<float>::infinity()) const
        requires bvh_intersectable<primitive_type>
    {
        if (m_primitives.empty())
        {
            return std::nullopt;
        }

        const vec3f inv_direction = vector::div(1.0f, ray.direction);

        const float root_distance = intersect_bounds(m_nodes[0], ray.origin, inv_direction);
        if (root_distance >= max_distance)
        {
            return std::nullopt;
        }

        std::array<stack_entry, traverse_stack_size> stack;
        std::size_t stack_size = 0;
        stack[stack_size++] = {root_distance, 0};

        float best_distance = max_distance;
        std::size_t best_index = BVH_INVALID_PRIMITIVE_INDEX;
        ray3f::hit_result best_hit = ray3f::no_hit;

        while (stack_size > 0)
        {
            const stack_entry current = stack[--stack_size];
            if (current.distance >= best_distance)
            {
                continue;
            }

            const node& n = m_nodes[current.index];

            if (n.is_leaf())
            {
                for (std::uint32_t i = 0; i < n.count; ++i)
                {
                    const std::uint32_t index = m_order[n.first + i];
                    const auto hit = m_primitives[index].intersect(ray);
                    if (hit && hit.enter < best_distance)
                    {
                        best_distance = hit.enter;
                        best_index = index;
                        best_hit = hit;
                    }
                }

                continue;
            }

            const std::uint32_t left = n.first;
            const std::uint32_t right = n.first + 1;

            const float left_distance = intersect_bounds(m_nodes[left], ray.origin, inv_direction);
            const float right_distance =
                intersect_bounds(m_nodes[right], ray.origin, inv_direction);

            // The farther child is pushed last so that the nearer one is popped first.
            if (left_distance < right_distance)
            {
                push_node(stack, stack_size, right_distance, right, best_distance);
                push_node(stack, stack_size, left_distance, left, best_distance);
            }
            else
            {
                push_node(stack, stack_size, left_distance, left, best_distance);
                push_node(stack, stack_size, right_distance, right, best_distance);
            }
        }

        if (best_index == BVH_INVALID_PRIMITIVE_INDEX)
        {
            return std::nullopt;
        }

        return std::make_pair(best_index, best_hit);
    }

    std::pair<std::size_t, float> get_distance(
        const vec3f& position,
        float max_distance = std::numeric_limits<float>::infinity()) const
        requires bvh_distance_computable<primitive_type> ||
                 bvh_distance_sq_computable<primitive_type>
    {
        if (m_primitives.empty())
        {
            return {BVH_INVALID_PRIMITIVE_INDEX, max_distance};
        }

        std::array<stack_entry, traverse_stack_size> stack;
        std::size_t stack_size = 0;
        stack[stack_size++] = {node_distance_sq(m_nodes[0], position), 0};

        float min_distance_sq = max_distance * max_distance;
        std::size_t primitive_index = BVH_INVALID_PRIMITIVE_INDEX;

        while (stack_size > 0)
        {
            const stack_entry current = stack[--stack_size];
            if (current.distance > min_distance_sq)
            {
                continue;
            }

            const node& n = m_nodes[current.index];

            if (n.is_leaf())
            {
                for (std::uint32_t i = 0; i < n.count; ++i)
                {
                    const std::uint32_t index = m_order[n.first + i];
                    const primitive_type& primitive = m_primitives[index];

                    float primitive_distance_sq;
                    if constexpr (bvh_distance_sq_computable<primitive_type>)
                    {
                        primitive_distance_sq = primitive.get_distance_sq(position);
                    }
                    else
                    {
                        const float primitive_distance = primitive.get_distance(position);
                        primitive_distance_sq = primitive_distance * primitive_distance;
                    }

                    if (primitive_distance_sq < min_distance_sq)
                    {
                        min_distance_sq = primitive_distance_sq;
                        primitive_index = index;
                    }
                }

                continue;
            }

            const std::uint32_t left = n.first;
            const std::uint32_t right = n.first + 1;

            const float left_distance_sq = node_distance_sq(m_nodes[left], position);
            const float right_distance_sq = node_distance_sq(m_nodes[right], position);

            if (left_distance_sq < right_distance_sq)
            {
                if (right_distance_sq < min_distance_sq)
                {
                    push_node(stack, stack_size, right_distance_sq, right, min_distance_sq);
                }

                if (left_distance_sq < min_distance_sq)
                {
                    push_node(stack, stack_size, left_distance_sq, left, min_distance_sq);
                }
            }
            else
            {
                if (left_distance_sq < min_distance_sq)
                {
                    push_node(stack, stack_size, left_distance_sq, left, min_distance_sq);
                }

                if (right_distance_sq < min_distance_sq)
                {
                    push_node(stack, stack_size, right_distance_sq, right, min_distance_sq);
                }
            }
        }

        return {
            primitive_index,
            primitive_index == BVH_INVALID_PRIMITIVE_INDEX ? max_distance :
                                                             std::sqrt(min_distance_sq),
        };
    }

private:
    static constexpr std::uint32_t bucket_count = 16;

    struct node
    {
        vec3f bounds_min{std::numeric_limits<float>::max()};

        // Internal node: the index of the left child, the right child is the next node.
        // Leaf: the offset of the first primitive in m_order.
        std::uint32_t first{0};

        vec3f bounds_max{std::numeric_limits<float>::lowest()};

        // Zero for an internal node, the primitive count for a leaf.
        std::uint32_t count{0};

        bool is_leaf() const noexcept
        {
            return count != 0;
        }
    };

    static_assert(sizeof(node) == 32);

    struct build_entry
    {
        vec3f centroid;
        std::uint32_t index;
        std::uint32_t bucket;
    };

    struct bucket
    {
        box3f bounds;
        std::uint32_t count{0};
    };

    struct split_result
    {
        node left;
        node right;
    };

    struct stack_entry
    {
        float distance;
        std::uint32_t index;
    };

    static node make_node(std::uint32_t first, std::uint32_t count, const box3f& bounds) noexcept
    {
        node result;
        result.bounds_min = bounds.min;
        result.bounds_max = bounds.max;
        result.first = first;
        result.count = count;
        return result;
    }

    static float intersect_bounds(
        const node& n,
        const vec3f& origin,
        const vec3f& inv_direction) noexcept
    {
        float t1 = (n.bounds_min.x - origin.x) * inv_direction.x;
        float t2 = (n.bounds_max.x - origin.x) * inv_direction.x;
        float enter = std::min(t1, t2);
        float exit = std::max(t1, t2);

        t1 = (n.bounds_min.y - origin.y) * inv_direction.y;
        t2 = (n.bounds_max.y - origin.y) * inv_direction.y;
        enter = std::max(enter, std::min(t1, t2));
        exit = std::min(exit, std::max(t1, t2));

        t1 = (n.bounds_min.z - origin.z) * inv_direction.z;
        t2 = (n.bounds_max.z - origin.z) * inv_direction.z;
        enter = std::max(enter, std::min(t1, t2));
        exit = std::min(exit, std::max(t1, t2));

        return exit >= enter ? enter : std::numeric_limits<float>::infinity();
    }

    static float node_distance_sq(const node& n, const vec3f& position) noexcept
    {
        const vec3f clamped = vector::clamp(position, n.bounds_min, n.bounds_max);
        return vector::length_sq(clamped - position);
    }

    static void push_node(
        std::array<stack_entry, traverse_stack_size>& stack,
        std::size_t& stack_size,
        float distance,
        std::uint32_t index,
        float max_distance) noexcept
    {
        if (distance >= max_distance)
        {
            return;
        }

        assert(stack_size < stack.size());
        stack[stack_size++] = {distance, index};
    }

    static std::uint32_t get_bucket_index(float key, float axis_min, float axis_length) noexcept
    {
        const auto index = static_cast<std::uint32_t>(
            static_cast<float>(bucket_count) * (key - axis_min) / axis_length);
        return std::min(index, bucket_count - 1);
    }

    split_result split(std::span<build_entry> entries, std::uint32_t depth) const
    {
        box3f centroid_bounds;
        for (const build_entry& entry : entries)
        {
            box::expand(centroid_bounds, entry.centroid);
        }

        const std::uint32_t axis = box::get_max_extent_axis(centroid_bounds);
        const float axis_min = centroid_bounds.min[axis];
        const float axis_length = centroid_bounds.max[axis] - axis_min;

        if (axis_length > std::numeric_limits<float>::epsilon() && depth < max_tree_depth)
        {
            std::array<bucket, bucket_count> buckets = {};

            for (build_entry& entry : entries)
            {
                entry.bucket = get_bucket_index(entry.centroid[axis], axis_min, axis_length);

                bucket& current = buckets[entry.bucket];
                ++current.count;
                box::expand(current.bounds, m_primitives[entry.index].get_bounds());
            }

            std::array<box3f, bucket_count> left_bounds;
            std::array<std::uint32_t, bucket_count> left_counts;

            left_bounds[0] = buckets[0].bounds;
            left_counts[0] = buckets[0].count;
            for (std::uint32_t i = 1; i < bucket_count; ++i)
            {
                left_bounds[i] = left_bounds[i - 1];
                box::expand(left_bounds[i], buckets[i].bounds);

                left_counts[i] = left_counts[i - 1] + buckets[i].count;
            }

            std::array<box3f, bucket_count> right_bounds;
            std::array<std::uint32_t, bucket_count> right_counts;

            right_bounds[bucket_count - 1] = buckets[bucket_count - 1].bounds;
            right_counts[bucket_count - 1] = buckets[bucket_count - 1].count;
            for (std::uint32_t i = bucket_count - 1; i-- > 0;)
            {
                right_bounds[i] = right_bounds[i + 1];
                box::expand(right_bounds[i], buckets[i].bounds);

                right_counts[i] = right_counts[i + 1] + buckets[i].count;
            }

            float best_cost = std::numeric_limits<float>::infinity();
            std::uint32_t best_split = 0;

            for (std::uint32_t i = 0; i < bucket_count - 1; ++i)
            {
                const float cost =
                    (static_cast<float>(left_counts[i]) * box::get_surface_area(left_bounds[i])) +
                    (static_cast<float>(right_counts[i + 1]) *
                     box::get_surface_area(right_bounds[i + 1]));
                if (cost < best_cost)
                {
                    best_cost = cost;
                    best_split = i;
                }
            }

            const auto middle = std::partition(
                entries.begin(),
                entries.end(),
                [best_split](const build_entry& entry)
                {
                    return entry.bucket <= best_split;
                });

            const auto split_offset =
                static_cast<std::size_t>(std::distance(entries.begin(), middle));

            if (split_offset > 0 && split_offset < entries.size())
            {
                assert(split_offset == left_counts[best_split]);

                return {
                    make_node(0, static_cast<std::uint32_t>(split_offset), left_bounds[best_split]),
                    make_node(
                        static_cast<std::uint32_t>(split_offset),
                        static_cast<std::uint32_t>(entries.size() - split_offset),
                        right_bounds[best_split + 1]),
                };
            }
        }

        const std::size_t split_offset = entries.size() / 2;

        std::nth_element(
            entries.begin(),
            entries.begin() + split_offset,
            entries.end(),
            [axis](const build_entry& a, const build_entry& b)
            {
                return a.centroid[axis] < b.centroid[axis];
            });

        box3f left_bounds;
        for (std::size_t i = 0; i < split_offset; ++i)
        {
            box::expand(left_bounds, m_primitives[entries[i].index].get_bounds());
        }

        box3f right_bounds;
        for (std::size_t i = split_offset; i < entries.size(); ++i)
        {
            box::expand(right_bounds, m_primitives[entries[i].index].get_bounds());
        }

        return {
            make_node(0, static_cast<std::uint32_t>(split_offset), left_bounds),
            make_node(
                static_cast<std::uint32_t>(split_offset),
                static_cast<std::uint32_t>(entries.size() - split_offset),
                right_bounds),
        };
    }

    std::vector<primitive_type> m_primitives;
    std::vector<std::uint32_t> m_order;
    std::vector<node> m_nodes;
};
} // namespace violet