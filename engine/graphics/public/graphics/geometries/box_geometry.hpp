#pragma once

#include "graphics/geometry.hpp"

namespace violet
{
class box_geometry : public geometry
{
public:
    box_geometry(
        float width = 1.0f,
        float height = 1.0f,
        float depth = 1.0f,
        std::size_t width_segments = 1,
        std::size_t height_segments = 1,
        std::size_t depth_segments = 1);
};
} // namespace violet