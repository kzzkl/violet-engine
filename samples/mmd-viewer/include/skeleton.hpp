#pragma once

#include "entity.hpp"
#include "render_parameter.hpp"
#include <vector>

namespace ash::sample::mmd
{
struct skeleton
{
    std::vector<ecs::entity> nodes;
    std::unique_ptr<ash::graphics::render_parameter> parameter;

    std::vector<math::float4x4> transform;
};
} // namespace ash::sample::mmd