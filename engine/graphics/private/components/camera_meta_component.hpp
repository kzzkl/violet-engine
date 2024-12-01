#pragma once

#include "components/camera_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
struct camera_meta_component
{
    rhi_ptr<rhi_parameter> parameter;

    render_camera data;
    std::vector<rhi_fence*> swapchain_fences;
};

template <>
struct component_trait<camera_meta_component>
{
    using main_component = camera_component;
};
} // namespace violet