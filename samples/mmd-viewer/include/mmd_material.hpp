#pragma once

#include "graphics/material.hpp"

namespace violet::sample
{
struct mmd_material_constant
{
};

class mmd_material : public mesh_material<mmd_material_constant>
{
public:
    mmd_material();

    void set_diffuse(const vec4f& diffuse);
    void set_specular(vec3f specular, float specular_strength);
    void set_ambient(const vec3f& ambient);
    void set_toon_mode(std::uint32_t toon_mode);
    void set_spa_mode(std::uint32_t spa_mode);

    void set_albedo(rhi_texture* texture);
    void set_toon(rhi_texture* texture);
    void set_spa(rhi_texture* texture);

    void set_edge(const vec4f& edge_color, float edge_size);
};
} // namespace violet::sample