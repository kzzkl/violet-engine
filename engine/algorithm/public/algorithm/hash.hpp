#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace violet
{
struct hash
{
    static inline std::uint64_t combine(std::uint64_t seed, std::uint64_t value)
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    static std::uint64_t city_hash_64(const void* data, std::size_t size);

    template <typename T>
    static std::uint64_t city_hash_64(const T& value) noexcept
    {
        return city_hash_64(&value, sizeof(T));
    }

    static std::uint32_t murmur_mix_32(std::uint32_t hash) noexcept
    {
        hash ^= hash >> 16;
        hash *= 0x85ebca6b;
        hash ^= hash >> 13;
        hash *= 0xc2b2ae35;
        hash ^= hash >> 16;

        return hash;
    }

    static std::uint32_t murmur_32(std::initializer_list<std::uint32_t> data) noexcept
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
};
} // namespace violet