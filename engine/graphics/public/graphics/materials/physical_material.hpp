#pragma once

#include "graphics/material.hpp"

namespace violet
{
class physical_material : public material
{
public:
    struct data
    {
        float3 albedo;
        float roughness;
        float metallic;
        float padding0;
        float padding1;
        float padding2;
    };

public:
    physical_material();

    // No texture version.
    void set_albedo(const float3& albedo);
    void set_roughness(float roughness);
    void set_metallic(float metallic);

    // Texture version.
    void set_albedo(rhi_texture* albedo, rhi_sampler* sampler);
    void set_roughness(rhi_texture* roughness, rhi_sampler* sampler);
    void set_metallic(rhi_texture* metallic, rhi_sampler* sampler);
    void set_normal(rhi_texture* normal, rhi_sampler* sampler);

    void set_irradiance_map(rhi_texture* irradiance_map, rhi_sampler* sampler);
    void set_prefilter_map(rhi_texture* prefilter_map, rhi_sampler* sampler);
    void set_brdf_lut(rhi_texture* brdf_lut, rhi_sampler* sampler);

private:
    void update_pso();

    rhi_texture* m_abledo_texture{nullptr};
    rhi_texture* m_roughness_texture{nullptr};
    rhi_texture* m_metallic_texture{nullptr};
    rhi_texture* m_normal_texture{nullptr};
};
} // namespace violet