#pragma once

#include <cstdint>

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

static constexpr entity INVALID_ENTITY = {
    .id = 0,
    .version = 0,
    .type = ENTITY_NULL,
};
} // namespace violet