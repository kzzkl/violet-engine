#pragma once

#include "component.hpp"
#include "render_parameter.hpp"
#include "transform.hpp"
#include <vector>

namespace ash::sample::mmd
{
struct skeleton
{
    std::vector<ecs::entity> nodes;
    std::unique_ptr<ash::graphics::render_parameter> parameter;

    std::vector<math::float4x4> offset;
};
} // namespace ash::sample::mmd