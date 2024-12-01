#pragma once

#include "graphics/renderer.hpp"
#include "math/math.hpp"
#include <variant>

namespace violet
{
struct camera_component
{
    float near{0.1f};
    float far{100000.0f};
    float fov{45.0f};

    float priority;

    renderer* renderer;
    std::vector<std::variant<rhi_texture*, rhi_swapchain*>> render_targets;

    rhi_texture_extent get_extent() const
    {
        rhi_texture_extent result = {};

        if (!render_targets.empty())
        {
            std::visit(
                [&result](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, rhi_swapchain*>)
                    {
                        result = arg->get_texture()->get_extent();
                    }
                    else if constexpr (std::is_same_v<T, rhi_texture*>)
                    {
                        result = arg->get_extent();
                    }
                },
                render_targets[0]);
        }

        return result;
    }
};
} // namespace violet