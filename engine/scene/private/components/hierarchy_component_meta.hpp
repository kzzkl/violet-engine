#pragma once

#include "components/hierarchy_component.hpp"
#include "ecs/component.hpp"

namespace violet
{
struct parent_component_meta
{
    entity previous_parent;
};

template <>
struct component_trait<parent_component_meta>
{
    using main_component = parent_component;
};
} // namespace violet