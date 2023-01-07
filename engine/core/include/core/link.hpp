#pragma once

#include "ecs/entity.hpp"
#include <vector>

namespace violet::core
{
struct link
{
    ecs::entity parent{ecs::INVALID_ENTITY};
    std::vector<ecs::entity> children;
};
} // namespace violet::core