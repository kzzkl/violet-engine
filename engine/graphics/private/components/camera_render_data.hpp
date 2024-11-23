#pragma once

#include "components/camera.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_scene.hpp"

namespace violet
{
struct camera_render_data
{
    rhi_ptr<rhi_parameter> parameter;

    render_camera data;
    std::vector<rhi_fence*> swapchain_fences;
};

template <>
struct component_trait<camera_render_data>
{
    using main_component = camera;
};
} // namespace violet