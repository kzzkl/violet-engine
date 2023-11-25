#pragma once

#include <cstddef>

namespace violet
{
inline std::size_t hash_combine(std::size_t seed, std::size_t value)
{
    return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
}
} // namespace violet