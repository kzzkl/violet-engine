#pragma once

#include "graphics/render_feature.hpp"
#include "graphics/renderer.hpp"
#include <variant>

namespace violet
{
struct camera_component
{
    float near{0.01f};
    float far{100000.0f};
    float fov{math::to_radians(45.0f)};

    float priority;

    renderer* renderer;
    std::variant<rhi_texture*, rhi_swapchain*> render_target;

    rhi_viewport viewport;
    std::vector<rhi_scissor_rect> scissor_rects;

    std::vector<std::unique_ptr<render_feature_base>> features;

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

    template <typename T>
    T* get_feature() const
    {
        for (const auto& feature : features)
        {
            if (feature->get_id() == render_feature_index::value<T>())
            {
                return static_cast<T*>(feature.get());
            }
        }

        return nullptr;
    }
};
} // namespace violet