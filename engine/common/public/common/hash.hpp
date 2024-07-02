#pragma once

#include <cstddef>
#include <cstdint>

namespace violet
{
struct hash
{
    static inline std::uint64_t combine(std::uint64_t seed, std::uint64_t value)
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    static std::uint64_t city_hash_64(const void* data, std::size_t size);
};
} // namespace violet