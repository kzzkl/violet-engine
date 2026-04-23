#include "atmosphere/atmosphere.hlsli"

struct constant_data
{
    atmosphere_data atmosphere;

    float3 sun_direction;
    uint sky_view_lut;
    float3 sun_irradiance;
    uint sample_count;
    float3 ground_color;
    uint transmittance_lut;
    uint multi_scattering_lut;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

[shader("compute")]
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

    float3 eye = camera.position + float3(0.0, atmosphere.planet_radius, 0.0);
    eye.y = max(eye.y, atmosphere.planet_radius + 100.0);

    if (!move_to_atmosphere(eye, view, atmosphere.atmosphere_radius))
    {
        sky_view_lut[dtid.xy] = 0.0;
        return;
    }

    float ground_distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.planet_radius);
    float distance = ground_distance;
    if (distance < 0.0)
    {
        distance = ray_sphere_intersection(eye, view, 0.0, atmosphere.atmosphere_radius);
    }

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];

#ifdef USE_MULTI_SCATTERING
    Texture2D<float3> multi_scattering_lut = ResourceDescriptorHeap[constant.multi_scattering_lut];
    float4 result = integrate_atmosphere(atmosphere, eye, view, constant.sun_direction, distance, constant.sample_count, transmittance_lut, multi_scattering_lut);
#else
    float4 result = integrate_atmosphere(atmosphere, eye, view, constant.sun_direction, distance, constant.sample_count, transmittance_lut);
#endif
    result.xyz *= constant.sun_irradiance;

    if (ground_distance >= 0.0)
    {
        float3 ground_position = eye + view * ground_distance;
        float3 ground_normal = normalize(ground_position);
        float ground_n_dot_l = saturate(dot(ground_normal, -constant.sun_direction));

        if (ground_n_dot_l > 0.0)
        {
            float3 ground_sample_position = ground_position + ground_normal;
            float ground_radius = length(ground_sample_position);
            float sun_mu = dot(ground_sample_position / ground_radius, -constant.sun_direction);

            float2 transmittance_uv;
            get_transmittance_lut_uv(
                atmosphere.planet_radius,
                atmosphere.atmosphere_radius,
                ground_radius,
                sun_mu,
                transmittance_uv);

            float3 sun_transmittance =
                transmittance_lut.SampleLevel(get_linear_clamp_sampler(), transmittance_uv, 0.0);
            float3 view_transmittance =
                integrate_transmittance(atmosphere, eye, view, ground_distance, constant.sample_count);

            float3 ground_radiance =
                constant.ground_color * ground_n_dot_l * sun_transmittance * constant.sun_irradiance / PI;
            result.xyz += ground_radiance * view_transmittance;
        }
    }

    sky_view_lut[dtid.xy] = result.xyz;
}