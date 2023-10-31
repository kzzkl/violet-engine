#include "violet_mesh.hlsl"
#include "violet_camera.hlsl"

struct vs_in
{
    [[vk::location(0)]] float3 position: POSITION;
    [[vk::location(1)]] float3 color: COLOR;
    [[vk::location(2)]] float2 uv: UV;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float2 uv: UV;
};

ConstantBuffer<violet_mesh> mesh : register(b0, space0);

[[vk::combinedImageSampler]]
Texture2D<float4> texture : register(t0, space1);
[[vk::combinedImageSampler]]
SamplerState texture_sampler : register(s0, space1);

ConstantBuffer<violet_camera> camera : register(b0, space2);

vs_out vs_main(vs_in input)
{
    vs_out output;

    output.position = mul(camera.view_projection, mul(mesh.model, float4(input.position, 1.0)));
    output.color = input.color;
    output.uv = input.uv;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    float4 color = float4(input.color, 1.0);
    color *= texture.Sample(texture_sampler, input.uv);
    return color;
}