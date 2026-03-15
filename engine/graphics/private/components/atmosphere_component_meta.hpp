#pragma once

#include "components/atmosphere_component.hpp"
#include "ecs/component.hpp"
#include "graphics/atmosphere.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
struct atmosphere_component_meta
{
    atmosphere atmosphere;

    rhi_ptr<rhi_texture> transmittance_lut;

    bool update(const atmosphere_component& atmosphere_component)
    {
        bool dirty =
            atmosphere.rayleigh_scattering != atmosphere_component.rayleigh_scattering ||
            atmosphere.rayleigh_density_height != atmosphere_component.rayleigh_density_height ||
            atmosphere.mie_scattering != atmosphere_component.mie_scattering ||
            atmosphere.mie_asymmetry != atmosphere_component.mie_asymmetry ||
            atmosphere.mie_absorption != atmosphere_component.mie_absorption ||
            atmosphere.mie_density_height != atmosphere_component.mie_density_height ||
            atmosphere.ozone_absorption != atmosphere_component.ozone_absorption ||
            atmosphere.ozone_center_height != atmosphere_component.ozone_center_height ||
            atmosphere.ozone_width != atmosphere_component.ozone_width ||
            atmosphere.planet_radius != atmosphere_component.planet_radius ||
            atmosphere.atmosphere_height != atmosphere_component.atmosphere_height;

        if (dirty)
        {
            atmosphere.rayleigh_scattering = atmosphere_component.rayleigh_scattering;
            atmosphere.rayleigh_density_height = atmosphere_component.rayleigh_density_height;
            atmosphere.mie_scattering = atmosphere_component.mie_scattering;
            atmosphere.mie_asymmetry = atmosphere_component.mie_asymmetry;
            atmosphere.mie_absorption = atmosphere_component.mie_absorption;
            atmosphere.mie_density_height = atmosphere_component.mie_density_height;
            atmosphere.ozone_absorption = atmosphere_component.ozone_absorption;
            atmosphere.ozone_center_height = atmosphere_component.ozone_center_height;
            atmosphere.ozone_width = atmosphere_component.ozone_width;
            atmosphere.planet_radius = atmosphere_component.planet_radius;
            atmosphere.atmosphere_height = atmosphere_component.atmosphere_height;
        }

        atmosphere.sun_angular_radius = atmosphere_component.sun_angular_radius;

        return dirty;
    }
};

template <>
struct component_trait<atmosphere_component_meta>
{
    using main_component = atmosphere_component;
};
} // namespace violet