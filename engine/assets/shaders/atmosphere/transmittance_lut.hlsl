#include "atmosphere/atmosphere.hlsli"

struct constant_data
{
    atmosphere_data atmosphere;

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

    atmosphere_data atmosphere = constant.atmosphere;

    float2 uv = get_compute_texcoord(dtid.xy, width, height);

    float r;
    float mu;
    get_transmittance_lut_r_mu(atmosphere.planet_radius, atmosphere.atmosphere_radius, uv, r, mu);

    float distance = ray_sphere_intersection(atmosphere.planet_radius, r, mu);
    if (distance == 0.0)
    {
        distance = ray_sphere_intersection(atmosphere.atmosphere_radius, r, mu);
    }

    float3 result = 0.0;
    float ds = distance / constant.sample_count;
    for (int i = 0; i < constant.sample_count; ++i)
    {
        float d_i = i * ds;
        float h = sqrt(d_i * d_i + 2.0 * r * mu * d_i + r * r) - atmosphere.planet_radius;

        float3 rayleigh = atmosphere.get_rayleigh_scattering(h);
        float mie = atmosphere.get_mie_extinction(h);
        float3 ozone = atmosphere.get_ozone_absorption(h);

        result += (rayleigh + mie + ozone) * ds;
    }

    result = exp(-result);
    transmittance_lut[dtid.xy] = result;
}