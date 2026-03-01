#include "common.hlsli"
#include "atmosphere/atmosphere.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

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
};
PushConstant(constant_data, constant);

struct vs_output
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

vs_output vs_main(uint vertex_id : SV_VertexID)
{
    const float3 vertices[8] = {
        float3(1.0, 1.0, 1.0),
        float3(1.0, 1.0, -1.0),
        float3(1.0, -1.0, -1.0),
        float3(1.0, -1.0, 1.0),
        float3(-1.0, 1.0, 1.0),
        float3(-1.0, 1.0, -1.0),
        float3(-1.0, -1.0, -1.0),
        float3(-1.0, -1.0, 1.0)};

    const int indexes[36] = {
        0, 2, 1,
        2, 0, 3,
        4, 1, 5,
        1, 4, 0,
        7, 5, 6,
        5, 7, 4,
        3, 6, 2,
        6, 3, 7,
        4, 3, 0,
        3, 4, 7,
        1, 6, 5,
        6, 1, 2}; // x+, y+, x-, y-, z+, z-*/

    float3 position = camera.position + vertices[indexes[vertex_id]] * 100.0;

    vs_output result;
    result.position = mul(camera.matrix_vp_no_jitter, float4(position, 1.0));
    result.position.z = 0.0;
    result.texcoord = normalize(vertices[indexes[vertex_id]]);

    return result;
}

float4 fs_main(vs_output input) : SV_TARGET
{
    float3 view_dir = normalize(input.texcoord);

#ifdef USE_ATMOSPHERIC_SCATTERING
    atmosphere atmosphere;
    atmosphere.rayleigh_scattering = constant.rayleigh_scattering;
    atmosphere.rayleigh_density_height = constant.rayleigh_density_height;
    atmosphere.mie_scattering = constant.mie_scattering;
    atmosphere.mie_asymmetry = constant.mie_asymmetry;
    atmosphere.mie_absorption = constant.mie_absorption;
    atmosphere.mie_density_height = constant.mie_density_height;
    atmosphere.ozone_absorption = constant.ozone_absorption;
    atmosphere.ozone_center_height = constant.ozone_center_height;
    atmosphere.ozone_width = constant.ozone_width;
    atmosphere.planet_radius = constant.planet_radius;
    atmosphere.atmosphere_height = constant.atmosphere_height;

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
    light_data light = lights[0];

    float3 eye = camera.position;
    eye.y = constant.planet_radius + 1.0;

    float distance = ray_sphere_intersection(eye, view_dir, 0.0, constant.planet_radius);
    if (distance > 0.0)
    {
        return float4(0.0, 0.0, 0.0, 1.0);
    }

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    int sample_count = 10;

    distance = ray_sphere_intersection(eye, view_dir, 0.0, constant.planet_radius + constant.atmosphere_height);
    float ds = distance / sample_count;

    float3 p = eye;
    float3 color = 0.0;
    float3 optical_depth = 0.0;
    for (int i = 0; i < sample_count; ++i)
    {
        p += view_dir * ds;

        float r = length(p);
        float mu = dot(normalize(p), -light.direction);

        float h = r - constant.planet_radius;
        float3 extinction = atmosphere.get_rayleigh_coefficient(h) + atmosphere.get_mie_coefficient(h) + atmosphere.get_ozone_absorption(h) + atmosphere.get_mie_absorption(h);
        optical_depth += extinction * ds;

        float2 uv;
        get_transmittance_lut_uv(constant.planet_radius, constant.planet_radius + constant.atmosphere_height, r, mu, uv);

        float3 t0 = transmittance_lut.Sample(linear_clamp_sampler, uv);
        float3 s = atmosphere.scattering(p, light.direction, -view_dir);
        float3 t1 = exp(-optical_depth);

        color += t0 * s * t1 * ds;
    }

    float3 sun = atmosphere.sun_disk(view_dir, light.direction, exp(-optical_depth));
    color *= light.color + sun;

    return float4(color, 1.0);
#else
    TextureCube<float4> sky_texture = ResourceDescriptorHeap[scene.skybox];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    return sky_texture.Sample(linear_clamp_sampler, view_dir);
#endif
}