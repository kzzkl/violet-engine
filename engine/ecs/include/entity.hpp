#pragma once

#include "component.hpp"
#include <cstdint>

namespace ash::ecs
{
using entity_id = std::uint32_t;

struct entity
{
};

template <>
struct component_trait<entity>
{
    static constexpr std::size_t id = uuid("42ec283a-6341-4ae8-8e5e-261c87cc6ef1").hash();
};
} // namespace ash::ecs