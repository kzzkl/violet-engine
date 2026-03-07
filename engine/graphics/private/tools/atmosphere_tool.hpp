#pragma once

#include "graphics/render_interface.hpp"
#include "math/types.hpp"

namespace violet
{
class atmosphere_tool
{
public:
    static void generate_transmittance_lut(
        vec3f rayleigh_scattering,
        float rayleigh_density_height,
        float mie_scattering,
        float mie_absorption,
        float mie_density_height,
        float ozone_center_height,
        vec3f ozone_absorption,
        float ozone_width,
        float planet_radius,
        float atmosphere_height,
        std::uint32_t sample_count,
        rhi_texture* transmittance_lut,
        rhi_command* command = nullptr);
};
} // namespace violet