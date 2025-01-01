#pragma once

#include "components/skinned_component.hpp"
#include "ecs/component.hpp"
#include "graphics/geometry.hpp"

namespace violet
{
struct skinned_meta_component
{
    geometry* original_geometry;
    std::unique_ptr<geometry> skinned_geometry;

    std::vector<rhi_ptr<rhi_buffer>> bone_buffers;
    std::size_t current_index = 0;
};

template <>
struct component_trait<skinned_meta_component>
{
    using main_component = skinned_component;
};
} // namespace violet