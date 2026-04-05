#pragma once

#include "graphics/render_device.hpp"

namespace violet
{
struct skybox_component
{
    rhi_ptr<rhi_texture> texture;
};
} // namespace violet