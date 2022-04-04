#pragma once

#include "assert.hpp"
#include "component.hpp"

namespace ash::graphics
{
class render_pipeline;
struct submesh
{
    render_pipeline* pipeline;
    render_unit unit;
};

struct visual
{
    std::vector<submesh> submesh;

    render_parameter* object;
    render_parameter* skeleton;
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