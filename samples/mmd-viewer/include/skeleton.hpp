#pragma once

#include "component.hpp"
#include "render_parameter.hpp"
#include "transform.hpp"
#include <vector>

namespace ash::sample::mmd
{
struct skeleton
{
    std::vector<ash::scene::transform_node*> nodes;
    std::unique_ptr<ash::graphics::render_parameter> parameter;

    std::vector<math::float4x4> offset;
};
} // namespace ash::sample::mmd

namespace ash::ecs
{
template <>
struct component_trait<ash::sample::mmd::skeleton>
{
    static constexpr std::size_t id = ash::uuid("be64e8c8-568e-4fdc-9d8c-d891c8ce2de9").hash();
};
} // namespace ash::ecs