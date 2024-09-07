#pragma once

#include "common/hash.hpp"
#include <cstdint>
#include <limits>
#include <string>

namespace violet
{
using entity_id = std::uint32_t;
using entity_version = std::uint16_t;

enum entity_type : std::uint8_t
{
    ENTITY_NULL,
    ENTITY_NORMAL,
    ENTITY_TEMPORARY,
};

struct entity
{
    entity_id id{0};
    entity_version version{0};
    entity_type type{ENTITY_NULL};

    bool operator==(const entity& other) const noexcept
    {
        return id == other.id && version == other.version && type == other.type;
    }
};

static constexpr entity entity_null = {0, 0, ENTITY_NULL};
} // namespace violet

namespace std
{
template <>
struct hash<violet::entity>
{
    std::size_t operator()(const violet::entity& entity) const noexcept
    {
        return violet::hash::city_hash_64(&entity, sizeof(violet::entity));
    }
};
} // namespace std