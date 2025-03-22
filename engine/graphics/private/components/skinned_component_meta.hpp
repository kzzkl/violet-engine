#pragma once

#include "components/skinned_component.hpp"
#include "ecs/component.hpp"
#include "graphics/geometry.hpp"

namespace violet
{
struct skinned_component_meta
{
    geometry* original_geometry;
    std::unique_ptr<geometry> skinned_geometry;

    std::vector<std::unique_ptr<structured_buffer>> bone_buffers;
    std::size_t current_index = 0;
};

template <>
struct component_trait<skinned_component_meta>
{
    using main_component = skinned_component;
};
} // namespace violet