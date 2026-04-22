#pragma once

#include "graphics/material.hpp"
#include "graphics/resources/texture.hpp"

namespace violet
{
struct pbr_material_constant
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

class pbr_material_deferred
    : public material_instance<pbr_material_constant, MATERIAL_PATH_DEFERRED>
{
public:
    pbr_material_deferred();

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

private:
    rhi_shader* get_vertex_shader(std::span<std::wstring> defines) const override;
    rhi_shader* get_fragment_shader(std::span<std::wstring> defines) const override;
};

class pbr_material_visibility
    : public material_instance<pbr_material_constant, MATERIAL_PATH_VISIBILITY>
{
public:
    pbr_material_visibility();

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

private:
    rhi_shader* get_resolve_shader(std::span<std::wstring> defines) const override;
};

using pbr_material = pbr_material_deferred;
} // namespace violet