#include "violet_common.hlsli"

ConstantBuffer<violet_camera> camera : register(b0, space0);
ConstantBuffer<violet_light> light : register(b0, space1);
ConstantBuffer<violet_mesh> mesh : register(b0, space2);

struct pbr_material
{
    float3 color;
    float roughness;
    float metallic;
};
ConstantBuffer<pbr_material> material : register(b0, space3);
TextureCube irradiance_texture : register(t1, space3);
SamplerState irradiance_sampler : register(s1, space3);
TextureCube prefilter_texture : register(t1, space3);
SamplerState prefilter_sampler : register(s1, space3);
Texture2D brdf_lut : register(t1, space3);
SamplerState brdf_lut_sampler : register(s1, space3);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.position = mul(camera.view_projection, mul(mesh.model, float4(input.position, 1.0)));
    output.normal = mul((float3x3)mesh.model, input.normal);

    return output;
}

float4 fs_main(vs_out input) : SV_TARGET
{
    return float4(0.0, 0.0, 0.0, 1.0);
}