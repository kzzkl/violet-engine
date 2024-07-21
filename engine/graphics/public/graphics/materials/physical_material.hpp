#pragma once

#include "graphics/material.hpp"

namespace violet
{
class physical_material : public material
{
public:
    struct data
    {
        float3 color;
        float roughness;
        float metallic;
        float padding0;
        float padding1;
        float padding2;
    };

public:
    physical_material();

    void set_color(const float3& color);
    void set_roughness(float roughness);
    void set_metallic(float metallic);

    void set_irradiance_map(rhi_texture* irradiance_map, rhi_sampler* sampler);
    void set_prefilter_map(rhi_texture* prefilter_map, rhi_sampler* sampler);
    void set_brdf_lut(rhi_texture* brdf_lut, rhi_sampler* sampler);
};
} // namespace violet