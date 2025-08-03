#pragma once

#include "algorithm/disjoint_set.hpp"
#include "algorithm/radix_sort.hpp"
#include "math/box.hpp"
#include <array>
#include <unordered_map>

namespace violet
{
class graph_linker
{
public:
    template <typename PositionFunctor>
    void build_external_links(
        disjoint_set<std::uint32_t>& disjoint_set,
        std::uint32_t min_island_size,
        const box3f& bounds,
        PositionFunctor&& get_position)
    {
        std::uint32_t element_count = disjoint_set.get_size();

        std::vector<std::uint32_t> sorted_to_index(element_count);
        {
            std::vector<std::uint32_t> sort_keys(element_count);
            for (std::uint32_t i = 0; i < element_count; ++i)
            {
                vec3f center =
                    (get_position(i) - bounds.min) / vector::max(bounds.max - bounds.min);

                std::uint32_t key = morton_code_3(static_cast<std::uint32_t>(center.x * 1023.0f));
                key |= morton_code_3(static_cast<std::uint32_t>(center.y * 1023.0f)) << 1;
                key |= morton_code_3(static_cast<std::uint32_t>(center.z * 1023.0f)) << 2;
                sort_keys[i] = key;
            }

            std::iota(sorted_to_index.begin(), sorted_to_index.end(), 0);
            radix_sort_32(
                sorted_to_index.begin(),
                sorted_to_index.end(),
                [&](std::uint32_t index)
                {
                    return sort_keys[index];
                });
        }

        std::unordered_map<std::uint32_t, std::uint32_t> island_size;

        std::vector<std::pair<std::uint32_t, std::uint32_t>> island_ranges(element_count);
        {
            std::uint32_t island_index = 0;
            std::uint32_t first_element = 0;
            for (std::uint32_t i = 0; i < element_count; ++i)
            {
                std::uint32_t current_island_index = disjoint_set.find(sorted_to_index[i]);
                ++island_size[current_island_index];

                if (current_island_index != island_index)
                {
                    for (std::uint32_t j = first_element; j < i; ++j)
                    {
                        island_ranges[j] = {first_element, i};
                    }

                    first_element = i;
                    island_index = current_island_index;
                }
            }

            for (std::uint32_t i = first_element; i < element_count; ++i)
            {
                island_ranges[i] = {first_element, element_count};
            }
        }

        for (std::uint32_t i = 0; i < element_count; ++i)
        {
            std::uint32_t element_index = sorted_to_index[i];   
            std::uint32_t island_index = disjoint_set.find(element_index);

            // if (island_ranges[i].second - island_ranges[i].first > min_island_size)
            if (island_size[island_index] > min_island_size)
            {
                continue;
            }

            vec3f center = get_position(element_index);

            static constexpr std::uint32_t MAX_LINKS = 5;

            std::array<std::uint32_t, MAX_LINKS> closest_elements;
            closest_elements.fill(~0u);

            std::array<float, MAX_LINKS> closest_distances;
            closest_distances.fill(std::numeric_limits<float>::max());

            for (int step = -1; step <= 1; step += 2)
            {
                std::uint32_t limit = step == -1 ? 0 : element_count - 1;
                std::uint32_t adjacency = i;

                for (std::uint32_t iterations = 0; iterations < 16; ++iterations)
                {
                    if (adjacency == limit)
                    {
                        break;
                    }

                    adjacency += step;

                    std::uint32_t adjacency_index = sorted_to_index[adjacency];
                    std::uint32_t adjacency_island_index = disjoint_set.find(adjacency_index);

                    if (adjacency_island_index == island_index)
                    {
                        adjacency = step == -1 ? island_ranges[adjacency].first :
                                                 island_ranges[adjacency].second - 1;
                    }
                    else
                    {
                        float distance = vector::length_sq(center - get_position(adjacency_index));
                        for (std::size_t link = 0; link < closest_distances.size(); ++link)
                        {
                            if (distance < closest_distances[link])
                            {
                                std::swap(distance, closest_distances[link]);
                                std::swap(adjacency_index, closest_elements[link]);
                            }
                        }
                    }
                }
            }

            for (std::uint32_t closest_element : closest_elements)
            {
                if (closest_element != ~0u)
                {
                    m_external_links.insert({element_index, closest_element});
                    m_external_links.insert({closest_element, element_index});
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
    static std::uint32_t morton_code_3(std::uint32_t x) noexcept
    {
        x &= 0x000003ff;
        x = (x ^ (x << 16)) & 0xff0000ff;
        x = (x ^ (x << 8)) & 0x0300f00f;
        x = (x ^ (x << 4)) & 0x030c30c3;
        x = (x ^ (x << 2)) & 0x09249249;
        return x;
    }

    std::unordered_multimap<std::uint32_t, std::uint32_t> m_external_links;
};
} // namespace violet