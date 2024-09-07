#pragma once

#include "graphics/renderer.hpp"
#include "math/math.hpp"
#include <variant>

namespace violet
{
struct camera
{
    float4x4 projection;
    rhi_viewport viewport;

    float priority;

    renderer* renderer;
    std::vector<std::variant<rhi_texture*, rhi_swapchain*>> render_targets;
};
} // namespace violet