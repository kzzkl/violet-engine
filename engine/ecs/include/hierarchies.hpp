#pragma once

#include "component.hpp"
#include "entity.hpp"

namespace ash::ecs
{
struct hierarchies
{
    entity parent;
    std::vector<entity> children;
};

template <>
struct component_trait<hierarchies>
{
    static constexpr std::size_t id = ash::uuid("42ec283a-6341-4ae8-8e5e-261c87cc6ef1").hash();
};
} // namespace ash::ecs