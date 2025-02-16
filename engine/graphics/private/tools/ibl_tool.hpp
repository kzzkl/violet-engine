#pragma once

#include "graphics/render_interface.hpp"

namespace violet
{
class ibl_tool
{
public:
    static void generate_cube_map(rhi_texture* env_map, rhi_texture* cube_map);

    static void generate_ibl(
        rhi_texture* cube_map,
        rhi_texture* irradiance_map,
        rhi_texture* prefilter_map);
};
} // namespace violet