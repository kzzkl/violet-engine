#pragma once

#include "math/types.hpp"
#include <algorithm>
#include <cmath>

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

    vec2f get_transmittance_lut_uv(const vec3f& eye, const vec3f& sun_direction) const
    {
        float r = std::max(eye.y + planet_radius, planet_radius + 1.0f);
        float mu = -sun_direction.y;

        return get_transmittance_lut_uv(r, mu);
    }

    vec2f get_transmittance_lut_uv(float r, float mu) const
    {
        float atmosphere_radius = planet_radius + atmosphere_height;

        float h =
            std::sqrt((atmosphere_radius * atmosphere_radius) - (planet_radius * planet_radius));
        float rho = std::sqrt((r * r) - (planet_radius * planet_radius));

        float discriminant = (r * r * (mu * mu - 1.0f)) + (atmosphere_radius * atmosphere_radius);
        float d = std::max(0.0f, ((-r * mu) + std::sqrt(discriminant)));

        float d_min = atmosphere_radius - r;
        float d_max = rho + h;

        float x_mu = (d - d_min) / (d_max - d_min);
        float x_r = rho / h;

        return {x_mu, x_r};
    }
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