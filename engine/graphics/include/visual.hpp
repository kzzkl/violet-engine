#pragma once

#include "render_group.hpp"

namespace ash::graphics
{
struct visual
{
    render_group* group;
    std::unique_ptr<render_parameter_object> parameter;
};
} // namespace ash::graphics

namespace ash::ecs
{
template <>
struct component_trait<ash::graphics::visual>
{
    static constexpr std::size_t id = ash::uuid("be64e8c8-568e-4fdc-9d8c-d891c8ce2de0").hash();
};
} // namespace ash::ecs