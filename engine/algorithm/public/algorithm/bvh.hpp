#pragma once

#include "math/box.hpp"
#include "math/ray.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <queue>
#include <span>
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

template <typename T>
    requires bvh_primitive<T>
class bvh
{
public:
    using primitive_type = T;

    bvh(std::size_t primitive_count = 0)
    {
        m_primitives.reserve(primitive_count);
        m_nodes.reserve(primitive_count * 2);
        m_nodes.resize(1);
    }

    void add_primitive(const primitive_type& primitive)
    {
        m_primitives.push_back(primitive);
        box::expand(m_nodes[0].bounds, primitive.bounds);
    }

    const primitive_type& get_primitive(std::size_t index) const
    {
        assert(index < m_primitives.size());
        return m_primitives[index];
    }

    void build()
    {
        node& root = m_nodes[0];
        root.count = m_primitives.size();

        std::size_t offset = 0;
        while (offset < m_nodes.size())
        {
            node& n = m_nodes[offset++];

            if (n.count <= 1)
            {
                continue;
            }

            std::span<primitive_type> primitives(m_primitives.data() + n.offset, n.count);

            auto [l, r] = split(primitives);

            n.left = m_nodes.size();
            n.right = m_nodes.size() + 1;

            l.offset += n.offset;
            r.offset += n.offset;

            m_nodes.push_back(l);
            m_nodes.push_back(r);
        }
    }

    std::vector<std::pair<std::size_t, ray3f::hit_result>> intersect(const ray3f& ray) const
        requires bvh_intersectable<primitive_type>
    {
        std::vector<std::pair<std::size_t, ray3f::hit_result>> result;

        std::queue<std::size_t> queue;
        queue.push(0);

        while (!queue.empty())
        {
            const node& n = m_nodes[queue.front()];
            queue.pop();

            if (!ray.intersect(n.bounds))
            {
                continue;
            }

            if (n.is_leaf())
            {
                auto hit_result = m_primitives[n.offset].intersect(ray);
                if (hit_result)
                {
                    result.push_back({n.offset, hit_result});
                }
            }
            else
            {
                queue.push(n.left);
                queue.push(n.right);
            }
        }

        return result;
    }

    std::pair<std::size_t, float> get_distance(const vec3f& position) const
        requires bvh_distance_computable<primitive_type> ||
                 bvh_distance_sq_computable<primitive_type>
    {
        std::vector<std::size_t> stack;
        stack.push_back(0);

        float min_distance_sq = std::numeric_limits<float>::infinity();
        std::size_t primitive_index = 0;

        auto process_node = [&](std::size_t index, const node& n)
        {
            if (n.is_leaf())
            {
                float primitive_distance_sq;
                if constexpr (bvh_distance_sq_computable<primitive_type>)
                {
                    primitive_distance_sq = m_primitives[n.offset].get_distance_sq(position);
                }
                else
                {
                    float primitive_distance = m_primitives[n.offset].get_distance(position);
                    primitive_distance_sq = primitive_distance * primitive_distance;
                }

                if (primitive_distance_sq < min_distance_sq)
                {
                    min_distance_sq = primitive_distance_sq;
                    primitive_index = n.offset;
                }
            }
            else
            {
                stack.push_back(index);
            }
        };

        while (!stack.empty())
        {
            const node& n = m_nodes[stack.back()];
            stack.pop_back();

            float distance_sq = box::get_distance_sq(n.bounds, position);
            if (distance_sq > min_distance_sq)
            {
                continue;
            }

            const node& l = m_nodes[n.left];
            const node& r = m_nodes[n.right];

            float left_distance_sq = box::get_distance_sq(l.bounds, position);
            float right_distance_sq = box::get_distance_sq(r.bounds, position);

            if (left_distance_sq < right_distance_sq)
            {
                if (right_distance_sq < min_distance_sq)
                {
                    process_node(n.right, r);
                }

                if (left_distance_sq < min_distance_sq)
                {
                    process_node(n.left, l);
                }
            }
            else
            {
                if (left_distance_sq < min_distance_sq)
                {
                    process_node(n.left, l);
                }

                if (right_distance_sq < min_distance_sq)
                {
                    process_node(n.right, r);
                }
            }
        }

        return {primitive_index, std::sqrt(min_distance_sq)};
    }

private:
    static constexpr std::size_t bucket_count = 16;

    struct node
    {
        box3f bounds;

        std::size_t offset;
        std::size_t count;

        std::size_t left{0};
        std::size_t right{0};

        bool is_leaf() const noexcept
        {
            return left == right;
        }
    };

    std::pair<node, node> split(std::span<primitive_type> primitives)
    {
        box3f node_bounds;
        box3f centroid_bounds;

        for (const auto& primitive : primitives)
        {
            box::expand(node_bounds, primitive.bounds);
            box::expand(centroid_bounds, primitive.centroid);
        }

        std::uint32_t axis = box::get_max_extent_axis(centroid_bounds);
        float axis_length = centroid_bounds.max[axis] - centroid_bounds.min[axis];

        if (axis_length < std::numeric_limits<float>::epsilon())
        {
            std::size_t mid = primitives.size() / 2;

            node left = {
                .offset = 0,
                .count = mid,
            };
            for (std::size_t i = 0; i < mid; ++i)
            {
                box::expand(left.bounds, primitives[i].bounds);
            }

            node right = {
                .offset = mid,
                .count = primitives.size() - mid,
            };
            for (std::size_t i = mid; i < primitives.size(); ++i)
            {
                box::expand(right.bounds, primitives[i].bounds);
            }

            return {left, right};
        }

        auto get_bucket_index = [&](const primitive_type& primitive) -> std::size_t
        {
            auto bucket_index = static_cast<std::size_t>(
                bucket_count * (primitive.centroid[axis] - centroid_bounds.min[axis]) /
                axis_length);
            return std::clamp(bucket_index, static_cast<std::size_t>(0), bucket_count - 1);
        };

        struct bucket
        {
            box3f bounds;
            std::uint32_t count;
        };

        std::array<bucket, bucket_count> buckets = {};

        for (const auto& primitive : primitives)
        {
            auto bucket_index = get_bucket_index(primitive);

            bucket& bucket = buckets[bucket_index];
            box::expand(bucket.bounds, primitive.bounds);
            ++bucket.count;
        }

        std::array<box3f, bucket_count> left_bounds;
        std::array<std::uint32_t, bucket_count> left_counts;

        left_bounds[0] = buckets[0].bounds;
        left_counts[0] = buckets[0].count;
        for (std::uint32_t i = 1; i < bucket_count; ++i)
        {
            left_bounds[i] = buckets[i].bounds;
            box::expand(left_bounds[i], left_bounds[i - 1]);

            left_counts[i] = buckets[i].count + left_counts[i - 1];
        }

        std::array<box3f, bucket_count> right_bounds;
        std::array<std::uint32_t, bucket_count> right_counts;

        right_bounds[bucket_count - 1] = buckets[bucket_count - 1].bounds;
        right_counts[bucket_count - 1] = buckets[bucket_count - 1].count;
        for (std::int32_t i = bucket_count - 2; i >= 0; --i)
        {
            right_bounds[i] = buckets[i].bounds;
            box::expand(right_bounds[i], right_bounds[i + 1]);

            right_counts[i] = buckets[i].count + right_counts[i + 1];
        }

        float best_cost = std::numeric_limits<float>::infinity();
        std::uint32_t best_split = 0;

        for (std::uint32_t i = 0; i < bucket_count - 1; ++i)
        {
            float cost =
                (static_cast<float>(left_counts[i]) * box::get_surface_area(left_bounds[i])) +
                (static_cast<float>(right_counts[i + 1]) *
                 box::get_surface_area(right_bounds[i + 1]));
            if (cost < best_cost)
            {
                best_cost = cost;
                best_split = i;
            }
        }

        auto partition_iter = std::ranges::partition(
            primitives,
            [&](const primitive_type& primitive) -> bool
            {
                return get_bucket_index(primitive) <= best_split;
            });

        std::size_t split_offset =
            std::ranges::distance(primitives.begin(), partition_iter.begin());
        return {
            {
                .bounds = left_bounds[best_split],
                .offset = 0,
                .count = split_offset,
            },
            {
                .bounds = right_bounds[best_split + 1],
                .offset = split_offset,
                .count = primitives.size() - split_offset,
            },
        };
    }

    std::vector<primitive_type> m_primitives;
    std::vector<node> m_nodes;
};
} // namespace violet