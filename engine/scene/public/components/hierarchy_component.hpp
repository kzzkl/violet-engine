#pragma once

#include "ecs/entity.hpp"
#include <vector>

namespace violet
{
struct previous_parent_component
{
    entity parent;
};

struct parent_component
{
    entity parent;
};

struct child_component
{
    std::vector<entity> children;
};
} // namespace violet