#include "atmosphere/atmosphere.hlsli"

struct constant_data
{
    atmosphere_data atmosphere;

    uint sky_view_lut;
    float3 sun_direction;
    uint transmittance_lut;
    float3 sun_irradiance;
    uint sample_count;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float3> sky_view_lut = ResourceDescriptorHeap[constant.sky_view_lut];

    uint width;
    uint height;
    sky_view_lut.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    atmosphere_data atmosphere = constant.atmosphere;

    float2 uv = get_compute_texcoord(dtid.xy, width, height);
    float3 view = get_sky_view_lut_direction(uv);

    float3 eye = float3(0.0, max(camera.position.y + atmosphere.planet_radius, atmosphere.planet_radius + 1.0), 0.0);

    float distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.planet_radius);
    if (distance < 0.0)
    {
        distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.atmosphere_radius);
    }

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float ds = distance / constant.sample_count;

    float cos_theta = dot(-view, constant.sun_direction);
    float3 rayleigh_phase = atmosphere.get_rayleigh_phase(cos_theta);
    float mie_phase = atmosphere.get_mie_phase(cos_theta);

    float3 p = eye;
    float3 in_scatter = 0.0;
    float3 optical_depth = 0.0;
    for (int i = 0; i < constant.sample_count; ++i)
    {
        p += view * ds;

        float r = length(p);
        float mu = dot(normalize(p), -constant.sun_direction);

        float h = r - atmosphere.planet_radius;

        float3 rayleigh_scattering = atmosphere.get_rayleigh_scattering(h);
        float mie_scattering = atmosphere.get_mie_scattering(h);

        float3 extinction = 0.0;
        extinction += rayleigh_scattering;
        extinction += mie_scattering + atmosphere.get_mie_absorption(h);
        extinction += atmosphere.get_ozone_absorption(h);
        optical_depth += extinction * ds;
        
        float2 uv;
        get_transmittance_lut_uv(atmosphere.planet_radius, atmosphere.atmosphere_radius, r, mu, uv);

        float3 t0 = transmittance_lut.SampleLevel(linear_clamp_sampler, uv, 0.0);
        float3 s = rayleigh_scattering * rayleigh_phase + mie_scattering * mie_phase;
        float3 t1 = exp(-optical_depth);

        in_scatter += t0 * s * t1 * ds;
    }

    sky_view_lut[dtid.xy] = in_scatter * constant.sun_irradiance;
}