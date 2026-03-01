#include "common.hlsli"
#include "atmosphere/atmosphere.hlsli"

struct constant_data
{
    float3 rayleigh_scattering;
    float rayleigh_density_height;

    float mie_scattering;
    float mie_asymmetry;
    float mie_absorption;
    float mie_density_height;

    float3 ozone_absorption;
    float ozone_center_height;
    float ozone_width;

    float planet_radius;
    float atmosphere_height;

    uint transmittance_lut;
    uint sample_count;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];

    uint width;
    uint height;
    transmittance_lut.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float2 uv = get_compute_texcoord(dtid.xy, width, height);

    float atmosphere_radius = constant.planet_radius + constant.atmosphere_height;

    float r;
    float mu;
    get_transmittance_lut_r_mu(constant.planet_radius, atmosphere_radius, uv, r, mu);

    float distance = ray_sphere_intersection(atmosphere_radius, r, mu);

    float3 result = 0.0;
    float ds = distance / constant.sample_count;
    for (int i = 0; i < constant.sample_count; ++i)
    {
        float d_i = i * ds;
        float h = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r) - constant.planet_radius;

        float3 rayleigh = constant.rayleigh_scattering * exp(-h / constant.rayleigh_density_height);
        float mie = (constant.mie_scattering + constant.mie_absorption) * exp(-h / constant.mie_density_height);
        float3 ozone = constant.ozone_absorption * max(0.0, 1.0 - 0.5 * abs(h - constant.ozone_center_height) / constant.ozone_width);

        result += (rayleigh + mie + ozone) * ds;
    }

    result = exp(-result);
    transmittance_lut[dtid.xy] = result;
}