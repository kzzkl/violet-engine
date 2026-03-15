#include "common.hlsli"
#include "atmosphere/atmosphere.hlsli"

struct constant_data
{
    float3 sun_direction;
    float sun_angular_radius;
    float3 sun_irradiance;
    float planet_radius;
    float atmosphere_radius;
    uint skybox_texture;
    uint sky_view_lut;
    uint transmittance_lut;
    float2 transmittance_lut_uv;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
#ifdef DYNAMIC_SKY
    float3 transmittance : TRANSMITTANCE;
#endif
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

#ifdef DYNAMIC_SKY
    // float r = max(camera.position.y + constant.planet_radius, constant.planet_radius + 1.0);
    // float mu = -constant.sun_direction.y;
    // float2 transmittance_uv;
    // get_transmittance_lut_uv(constant.planet_radius, constant.atmosphere_radius, r, mu, transmittance_uv);
    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    result.transmittance = transmittance_lut.SampleLevel(get_linear_clamp_sampler(), constant.transmittance_lut_uv, 0.0);
#endif

    return result;
}

float4 fs_main(vs_output input) : SV_TARGET
{
    float3 view = normalize(input.texcoord);

#ifdef DYNAMIC_SKY
    Texture2D<float3> sky_view_lut = ResourceDescriptorHeap[constant.sky_view_lut];

    float2 uv = get_sky_view_lut_uv(view);
    float3 sky = sky_view_lut.SampleLevel(get_linear_clamp_sampler(), uv, 0.0);

    float3 eye = camera.position;
    eye.y = max(camera.position.y + constant.planet_radius, constant.planet_radius + 1.0);

    float3 sun = 0.0;
    if (ray_sphere_intersection(eye, view, 0.0, constant.planet_radius) < 0.0)
    {
        float cos_theta = dot(view, -constant.sun_direction);
        float cos_radius = cos(constant.sun_angular_radius);
        sun = smoothstep(cos_radius, cos_radius * 1.00001, cos_theta) * input.transmittance;
    }

    return float4(sky + sun * constant.sun_irradiance, 1.0);
#else
    TextureCube<float4> sky_texture = ResourceDescriptorHeap[constant.skybox_texture];
    return sky_texture.Sample(get_linear_clamp_sampler(), view);
#endif
}