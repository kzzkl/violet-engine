#include "violet_mesh.hlsl"
#include "violet_camera.hlsl"

ConstantBuffer<violet_camera> camera : register(b0, space0);
ConstantBuffer<violet_mesh> mesh : register(b0, space2);
Texture2D hdr_texture : register(t0, space3);
SamplerState hdr_sampler : register(s0, space3);

struct vs_in
{
    float3 position : POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_position: WORLD_POSITION;
};

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.world_position = input.position;
    output.position = mul(camera.view_projection, mul(mesh.model, float4(input.position, 1.0)));

    return output;
}

float2 sample_spherical_map(float3 v)
{
    const float2 inv_atan = {0.1591, -0.3183};
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    return uv;
}

float4 fs_main(vs_out input) : SV_TARGET
{
    float3 normal = normalize(input.world_position);
    float2 uv = sample_spherical_map(normal);
    return hdr_texture.Sample(hdr_sampler, uv);
}