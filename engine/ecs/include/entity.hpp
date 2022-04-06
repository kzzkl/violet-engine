#pragma once

#include "component.hpp"
#include <cstdint>

namespace ash::ecs
{
static constexpr std::uint32_t INVALID_ENTITY_ID = -1;

struct entity
{
    std::uint32_t id;
    std::uint32_t version;
};

static constexpr entity INVALID_ENTITY = {INVALID_ENTITY_ID, 0};

struct all_entity
{
};

template <>
struct component_trait<all_entity>
{
    static constexpr std::size_t id = uuid("42ec283a-6341-4ae8-8e5e-261c87cc6ef1").hash();
};
} // namespace ash::ecs