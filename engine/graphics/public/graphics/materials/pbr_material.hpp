#pragma once

#include "graphics/material.hpp"

namespace violet
{
struct pbr_material_constant
{
    vec3f albedo;
    float roughness;
    float metallic;
    float padding0;
    float padding1;
    float padding2;
};

class pbr_material : public mesh_material<pbr_material_constant>
{
public:
    pbr_material();

    void set_albedo(const vec3f& albedo);
    void set_albedo(rhi_texture* albedo);

    void set_roughness(float roughness);
    void set_roughness(rhi_texture* roughness);

    void set_metallic(float metallic);
    void set_metallic(rhi_texture* metallic);

    void set_normal(rhi_texture* normal);

private:
    rhi_texture* m_abledo_texture{nullptr};
    rhi_texture* m_roughness_texture{nullptr};
    rhi_texture* m_metallic_texture{nullptr};
    rhi_texture* m_normal_texture{nullptr};
};
} // namespace violet