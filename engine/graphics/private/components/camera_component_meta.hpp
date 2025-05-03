#pragma once

#include "components/camera_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
class camera_component_meta
{
public:
    mat4f matrix_v;
    mat4f matrix_p;
    mat4f matrix_vp;
    mat4f matrix_vp_no_jitter;

    rhi_ptr<rhi_texture> hzb;
    rhi_ptr<rhi_parameter> parameter;
};

template <>
struct component_trait<camera_component_meta>
{
    using main_component = camera_component;
};
} // namespace violet