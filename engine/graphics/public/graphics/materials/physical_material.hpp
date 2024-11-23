#pragma once

#include "graphics/material.hpp"

namespace violet
{
struct physical_material_constant
{
    vec3f albedo;
    float roughness;
    float metallic;
    float padding0;
    float padding1;
    float padding2;
};

class physical_material : public mesh_material<physical_material_constant>
{
public:
    physical_material();

    void set_albedo(const vec3f& albedo);
    void set_albedo(rhi_texture* albedo, rhi_sampler* sampler);

    void set_roughness(float roughness);
    void set_roughness(rhi_texture* roughness, rhi_sampler* sampler);

    void set_metallic(float metallic);
    void set_metallic(rhi_texture* metallic, rhi_sampler* sampler);

    void set_normal(rhi_texture* normal, rhi_sampler* sampler);

private:
    rhi_texture* m_abledo_texture{nullptr};
    rhi_texture* m_roughness_texture{nullptr};
    rhi_texture* m_metallic_texture{nullptr};
    rhi_texture* m_normal_texture{nullptr};
};
} // namespace violet