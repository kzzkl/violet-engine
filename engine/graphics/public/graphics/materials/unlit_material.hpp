#pragma once

#include "graphics/material.hpp"

namespace violet
{
struct unlit_material_constant
{
    vec3f color;
};

class unlit_material : public material_instance<unlit_material_constant>
{
public:
    unlit_material();

    void set_color(const vec3f& color);
};
} // namespace violet