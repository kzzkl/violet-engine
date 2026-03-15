#pragma once

#include "components/skybox_component.hpp"
#include "ecs/component.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
struct skybox_component_meta
{
    std::string environment_map_path;

    rhi_ptr<rhi_texture> environment_map;
    rhi_ptr<rhi_buffer> irradiance_sh;
    rhi_ptr<rhi_texture> prefilter_map;
};

template <>
struct component_trait<skybox_component_meta>
{
    using main_component = skybox_component;
};
} // namespace violet