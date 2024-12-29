#pragma once

#include "graphics/material.hpp"

namespace violet
{
struct mmd_material_constant
{
    vec4f diffuse;
    vec3f specular;
    float specular_strength;
    vec3f ambient;
    std::uint32_t diffuse_texture;
    std::uint32_t toon_texture;
    std::uint32_t environment_texture;
    std::uint32_t environment_blend_mode;
};

class mmd_material : public mesh_material<mmd_material_constant>
{
public:
    mmd_material();

    void set_diffuse(const vec4f& diffuse);
    void set_specular(vec3f specular, float specular_strength);
    void set_ambient(const vec3f& ambient);

    void set_diffuse(rhi_texture* texture);
    void set_toon(rhi_texture* texture);
    void set_environment(rhi_texture* texture);
    void set_environment_blend(std::uint32_t mode);
};

struct mmd_outline_material_constant
{
    vec3f color;
    float width;
};

class mmd_outline_material : public mesh_material<mmd_outline_material_constant>
{
public:
    mmd_outline_material();

    void set_outline(const vec4f& color, float width);
};
} // namespace violet