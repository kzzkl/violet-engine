#pragma once

#include "entity.hpp"
#include <vector>

namespace ash::core
{
struct link
{
    ash::ecs::entity parent{ecs::INVALID_ENTITY};
    std::vector<ash::ecs::entity> children;
};
} // namespace ash