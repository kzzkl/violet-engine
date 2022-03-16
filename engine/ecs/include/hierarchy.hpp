#pragma once

#include "component.hpp"
#include "entity.hpp"

namespace ash::ecs
{
struct hierarchy
{
    entity_id parent;
    std::vector<entity_id> children;
};

template <>
struct component_trait<hierarchy>
{
    static constexpr std::size_t id = ash::uuid("42ec283a-6341-4ae8-8e5e-261c87cc6ef1").hash();
};
} // namespace ash::ecs