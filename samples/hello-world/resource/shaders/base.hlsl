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

struct violet_mesh
{
	float4x4 model;
};

cbuffer mesh : register(b0, space0) { violet_mesh mesh; }

[[vk::combinedImageSampler]]
Texture2D<float4> texture : register(t0, space1);
[[vk::combinedImageSampler]]
SamplerState texture_sampler : register(s0, space1);

struct violet_camera
{
    float4x4 view;
    float4x4 project;
    float4x4 view_projection;
};

cbuffer camera : register(b0, space2) { violet_camera camera; }

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