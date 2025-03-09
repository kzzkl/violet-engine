#pragma once

#include "graphics/material.hpp"
#include "graphics/resources/texture.hpp"

namespace violet
{
struct physical_material_constant
{
    vec3f albedo;
    std::uint32_t albedo_texture;
    float roughness;
    std::uint32_t roughness_metallic_texture;
    float metallic;
    std::uint32_t normal_texture;
    vec3f emissive;
    std::uint32_t emissive_texture;
};

class physical_material : public mesh_material<physical_material_constant>
{
public:
    physical_material();

    void set_albedo(const vec3f& albedo);
    void set_albedo(texture_2d* albedo);
    vec3f get_albedo() const;

    void set_roughness(float roughness);
    float get_roughness() const;

    void set_metallic(float metallic);
    float get_metallic() const;

    void set_roughness_metallic(texture_2d* roughness_metallic);

    void set_emissive(const vec3f& emissive);
    void set_emissive(texture_2d* emissive);
    vec3f get_emissive() const;

    void set_normal(texture_2d* normal);
};
} // namespace violet