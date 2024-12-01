#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
struct skybox_component
{
    rhi_texture* texture;
    rhi_texture* irradiance;
    rhi_texture* prefilter;
};
} // namespace violet