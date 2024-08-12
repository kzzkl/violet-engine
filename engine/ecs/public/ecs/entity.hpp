#pragma once

#include <cstdint>
#include <limits>

namespace violet
{
using entity_id = std::uint32_t;
static constexpr entity_id entity_null = std::numeric_limits<entity_id>::max();

struct entity
{
    entity_id id{entity_null};
    std::uint32_t version{0};
};
} // namespace violet