#pragma once

#include "components/camera_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
struct camera_meta_component
{
    mat4f view_projection;
    mat4f view_projection_no_jitter;

    rhi_ptr<rhi_parameter> parameter;

    std::vector<rhi_fence*> swapchain_fences;
};

template <>
struct component_trait<camera_meta_component>
{
    using main_component = camera_component;
};
} // namespace violet