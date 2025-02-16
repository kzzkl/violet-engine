#pragma once

#include "graphics/material.hpp"
#include "graphics/resources/texture.hpp"

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
    std::uint32_t ramp_texture;
};

class mmd_material : public mesh_material<mmd_material_constant>
{
public:
    mmd_material();

    void set_diffuse(const vec4f& diffuse);
    void set_specular(vec3f specular, float specular_strength);
    void set_ambient(const vec3f& ambient);

    void set_diffuse(const texture_2d* texture);
    void set_toon(const texture_2d* texture);
    void set_environment(const texture_2d* texture);
    void set_environment_blend(std::uint32_t mode);

    void set_ramp_texture(const texture_2d* texture);
};

struct mmd_outline_material_constant
{
    vec3f color;
    float width;
    float z_offset;
};

class mmd_outline_material : public mesh_material<mmd_outline_material_constant>
{
public:
    mmd_outline_material();

    void set_color(const vec4f& color);
    void set_width(float width);
    void set_z_offset(float z_offset);
};
} // namespace violet