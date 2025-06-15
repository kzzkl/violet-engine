#pragma once

#include "graphics/geometry.hpp"

namespace violet
{
class sphere_geometry : public geometry
{
public:
    sphere_geometry(
        float radius = 0.5f,
        std::size_t width_segments = 32,
        std::size_t height_segments = 16,
        float phi_start = 0.0f,
        float phi_length = math::PI * 2.0f,
        float theta_start = 0.0f,
        float theta_length = math::PI);
};
} // namespace violet