#pragma once

#include "graphics/render_interface.hpp"
#include "math/math.hpp"
#include <memory>
#include <variant>
#include <vector>

namespace violet
{
enum camera_type
{
    CAMERA_PERSPECTIVE = 0,
    CAMERA_ORTHOGRAPHIC = 1,
};

class renderer;
class camera_component
{
public:
    camera_type type{camera_type::CAMERA_PERSPECTIVE};

    struct
    {
        float size{100.0f};
    } orthographic;

    struct
    {
        float fov{math::to_radians(60.0f)};
    } perspective;

    float near{0.01f};
    float far{std::numeric_limits<float>::infinity()};

    float priority{0.0f};

    std::unique_ptr<renderer> renderer;
    std::variant<rhi_texture*, rhi_swapchain*> render_target;

    rhi_viewport viewport;
    std::vector<rhi_scissor_rect> scissor_rects;

    bool has_render_target() const
    {
        bool result = false;

        std::visit(
            [&result](auto&& arg)
            {
                result = arg != nullptr;
            },
            render_target);

        return result;
    }

    rhi_texture_extent get_extent() const
    {
        rhi_texture_extent result = {};

        std::visit(
            [&result](auto&& arg)
            {
                if (arg == nullptr)
                {
                    return;
                }

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
            render_target);

        return result;
    }
};
} // namespace violet