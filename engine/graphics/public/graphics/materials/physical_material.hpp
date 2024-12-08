#pragma once

#include "graphics/material.hpp"

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
    void set_albedo(rhi_texture* albedo);

    void set_roughness(float roughness);
    void set_metallic(float metallic);
    void set_roughness_metallic(rhi_texture* roughness_metallic);

    void set_emissive(const vec3f& emissive);
    void set_emissive(rhi_texture* emissive);

    void set_normal(rhi_texture* normal);
};
} // namespace violet