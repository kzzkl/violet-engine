#pragma once

#include "graphics/render_interface.hpp"

namespace violet
{
struct skybox
{
    rhi_texture* texture;
    rhi_sampler* sampler;
};
} // namespace violet