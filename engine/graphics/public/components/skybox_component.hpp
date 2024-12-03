#pragma once

#include "graphics/render_interface.hpp"

namespace violet
{
struct skybox_component
{
    rhi_texture* texture;
    rhi_texture* irradiance;
    rhi_texture* prefilter;
};
} // namespace violet