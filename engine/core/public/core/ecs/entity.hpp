#pragma once

#include <cstdint>

namespace violet
{
static constexpr std::uint32_t INVALID_ENTITY_INDEX = -1;

struct entity
{
    std::uint32_t index = INVALID_ENTITY_INDEX;
    std::uint16_t entity_version = 0;
    std::uint16_t component_version = 0;
};
} // namespace violet