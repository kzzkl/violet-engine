#pragma once

#include "math/types.hpp"

namespace violet
{
struct atmosphere
{
    vec3f rayleigh_scattering;
    float rayleigh_density_height;

    float mie_scattering;
    float mie_asymmetry;
    float mie_absorption;
    float mie_density_height;

    vec3f ozone_absorption;
    float ozone_center_height;
    float ozone_width;

    float planet_radius;
    float atmosphere_height;

    float sun_angular_radius;
};

struct atmosphere_data
{
    vec3f rayleigh_scattering;
    float rayleigh_density_height;
    float mie_scattering;
    float mie_asymmetry;
    float mie_absorption;
    float mie_density_height;
    vec3f ozone_absorption;
    float ozone_center_height;
    float ozone_width;
    float planet_radius;
    float atmosphere_radius;
    float sun_angular_radius;

    atmosphere_data& operator=(const atmosphere& other)
    {
        rayleigh_scattering = other.rayleigh_scattering;
        rayleigh_density_height = other.rayleigh_density_height;
        mie_scattering = other.mie_scattering;
        mie_asymmetry = other.mie_asymmetry;
        mie_absorption = other.mie_absorption;
        mie_density_height = other.mie_density_height;
        ozone_absorption = other.ozone_absorption;
        ozone_center_height = other.ozone_center_height;
        ozone_width = other.ozone_width;
        planet_radius = other.planet_radius;
        atmosphere_radius = other.planet_radius + other.atmosphere_height;
        sun_angular_radius = other.sun_angular_radius;
        return *this;
    }
};
} // namespace violet