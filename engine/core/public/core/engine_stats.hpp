#pragma once

#include <cstddef>

namespace violet
{
struct engine_stats
{
    float delta{0.0f};
    std::size_t loop_count{0};
};
} // namespace violet