#pragma once

#include "math/types.hpp"

namespace violet
{
struct atmosphere
{
    vec3f rayleigh_scattering{5.802e-6f, 13.558e-6f, 33.1e-6f};
    float rayleigh_density_height{8000.0f};

    float mie_scattering{3.996e-6f};
    float mie_asymmetry{0.8e-6f};
    float mie_absorption{4.4e-6f};
    float mie_density_height{1200.0f};

    vec3f ozone_absorption{0.65e-6f, 1.881e-6f, 0.085e-6f};
    float ozone_center_height{25000.0f};
    float ozone_width{30000.0f};

    float planet_radius{6360000.0f};
    float atmosphere_height{100000.0f};
};
} // namespace violet