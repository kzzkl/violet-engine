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
    float atmosphere_radius;

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

    float2 uv = get_compute_texcoord(dtid.xy, width, height);
    float3 view = get_sky_view_lut_direction(uv);

    float3 eye = float3(0.0, max(camera.position.y + constant.planet_radius, constant.planet_radius + 1.0), 0.0);

    float distance = ray_sphere_intersection(eye, view, 0.0, constant.planet_radius);
    if (distance < 0.0)
    {
        distance = ray_sphere_intersection(eye, view, 0.0, constant.atmosphere_radius);
    }

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float ds = distance / constant.sample_count;

    float cos_theta = dot(-view, constant.sun_direction);
    float3 rayleigh_phase = get_rayleigh_phase(cos_theta);
    float mie_phase = get_mie_phase(cos_theta, constant.mie_asymmetry);

    float3 p = eye;
    float3 in_scatter = 0.0;
    float3 optical_depth = 0.0;
    for (int i = 0; i < constant.sample_count; ++i)
    {
        p += view * ds;

        float r = length(p);
        float mu = dot(normalize(p), -constant.sun_direction);

        float h = r - constant.planet_radius;

        float3 rayleigh_scattering = get_rayleigh_scattering(h, constant.rayleigh_density_height, constant.rayleigh_scattering);
        float mie_scattering = get_mie_scattering(h, constant.mie_density_height, constant.mie_scattering);

        float3 extinction = 0.0;
        extinction += rayleigh_scattering;
        extinction += mie_scattering + get_mie_absorption(h, constant.mie_density_height, constant.mie_absorption);
        extinction += get_ozone_absorption(h, constant.ozone_center_height, constant.ozone_width, constant.ozone_absorption);
        optical_depth += extinction * ds;

        float2 uv;
        get_transmittance_lut_uv(constant.planet_radius, constant.atmosphere_radius, r, mu, uv);

        float3 t0 = transmittance_lut.SampleLevel(linear_clamp_sampler, uv, 0.0);
        float3 s = rayleigh_scattering * rayleigh_phase + mie_scattering * mie_phase;
        float3 t1 = exp(-optical_depth);

        in_scatter += t0 * s * t1 * ds;
    }

    sky_view_lut[dtid.xy] = in_scatter * constant.sun_irradiance;
}