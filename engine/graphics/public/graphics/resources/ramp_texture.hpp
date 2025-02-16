#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class ramp_texture : public texture_2d
{
public:
    struct point
    {
        vec3f color;
        float position;
    };

    ramp_texture(const std::vector<point>& points, std::uint32_t width);
};
} // namespace violet