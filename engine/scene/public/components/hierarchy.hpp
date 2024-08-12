#pragma once

#include "ecs/entity.hpp"
#include <vector>

namespace violet
{
struct hierarchy_parent
{
    entity parent;
};

struct hierarchy_child
{
    std::vector<entity> children;
};
} // namespace violet