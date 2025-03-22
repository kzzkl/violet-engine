#pragma once

#include "math/types.hpp"
#include <span>
#include <vector>

namespace violet
{
struct cluster
{
    std::uint32_t index_offset;
    std::uint32_t index_count;
};

struct cluster_result
{
    std::vector<std::uint32_t> indexes;
    std::vector<cluster> clusters;
};

class cluster_tool
{
public:
    static cluster_result generate_clusters(
        std::span<const vec3f> positions,
        std::span<const std::uint32_t> indexes);
};
} // namespace violet