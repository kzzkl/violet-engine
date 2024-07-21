#include "graphics/materials/physical_material.hpp"

namespace violet
{

physical_material::physical_material()
{
}

void physical_material::set_color(const float3& color)
{
    get_parameter(0)->set_uniform(0, &color, sizeof(float3), offsetof(data, color));
}

void physical_material::set_roughness(float roughness)
{
    get_parameter(0)->set_uniform(0, &roughness, sizeof(float), offsetof(data, roughness));
}

void physical_material::set_metallic(float metallic)
{
    get_parameter(0)->set_uniform(0, &metallic, sizeof(float), offsetof(data, metallic));
}

void physical_material::set_irradiance_map(rhi_texture* irradiance_map, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(1, irradiance_map, sampler);
}

void physical_material::set_prefilter_map(rhi_texture* prefilter_map, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(2, prefilter_map, sampler);
}

void physical_material::set_brdf_lut(rhi_texture* brdf_lut, rhi_sampler* sampler)
{
    get_parameter(0)->set_texture(3, brdf_lut, sampler);
}
} // namespace violet