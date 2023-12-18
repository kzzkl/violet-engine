#pragma once

#include "graphics/geometry.hpp"

namespace violet
{
class sphere_geometry : public geometry
{
public:
    sphere_geometry(
        renderer* renderer,
        float radius = 0.5f,
        std::size_t slice = 30,
        std::size_t stack = 30);
};
} // namespace violet