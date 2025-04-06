#pragma once

#include "graphics/geometry.hpp"

namespace violet
{
class plane_geometry : public geometry
{
public:
    plane_geometry(
        float width = 1.0f,
        float height = 1.0f,
        std::size_t width_segments = 1,
        std::size_t height_segments = 1);
};
} // namespace violet