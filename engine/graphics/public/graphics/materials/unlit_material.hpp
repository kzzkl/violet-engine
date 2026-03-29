#pragma once

#include "graphics/material.hpp"

namespace violet
{
struct unlit_material_constant
{
    vec3f color;
};

class unlit_material : public material_instance<unlit_material_constant, MATERIAL_PATH_VISIBILITY>
{
public:
    unlit_material();

    void set_color(const vec3f& color);

private:
    rhi_shader* get_resolve_shader(std::span<std::wstring> defines) const override;
};
} // namespace violet