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

// class unlit_material_visibility
//     : public material_instance<unlit_material_constant, MATERIAL_PATH_VISIBILITY>
// {
// public:
//     unlit_material_visibility();

//     void set_color(const vec3f& color);

// private:
//     rhi_shader* get_resolve_shader(std::span<std::wstring> defines) const override;
// };

// using unlit_material = unlit_material_deferred;
} // namespace violet