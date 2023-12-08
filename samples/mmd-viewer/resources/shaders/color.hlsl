#include "violet_mesh.hlsl"
#include "violet_camera.hlsl"
#include "violet_light.hlsl"

struct vs_in
{
    [[vk::location(0)]] float3 position: POSITION;
    [[vk::location(1)]] float3 normal: NORMAL;
    [[vk::location(2)]] float2 uv: UV;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_normal: WORLD_NORMAL;
    float3 screen_normal: SCEEN_NORMAL;
    float2 uv: UV;
};

ConstantBuffer<violet_mesh> mesh : register(b0, space0);

struct mmd_material
{
    float4 diffuse;
    float3 specular;
    float specular_strength;
    float4 edge_color;
    float3 ambient;
    float edge_size;
    uint toon_mode;
    uint spa_mode;
};

ConstantBuffer<mmd_material> material : register(b0, space1);

[[vk::combinedImageSampler]]
Texture2D<float4> tex : register(t1, space1);
[[vk::combinedImageSampler]]
SamplerState tex_sampler : register(s1, space1);

[[vk::combinedImageSampler]]
Texture2D<float4> toon : register(t2, space1);
[[vk::combinedImageSampler]]
SamplerState toon_sampler : register(s2, space1);

[[vk::combinedImageSampler]]
Texture2D<float4> spa : register(t3, space1);
[[vk::combinedImageSampler]]
SamplerState spa_sampler : register(s3, space1);

ConstantBuffer<violet_camera> camera : register(b0, space2);
ConstantBuffer<violet_light> light : register(b0, space3);

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.position = mul(camera.view_projection, mul(mesh.model, float4(input.position, 1.0)));
    output.world_normal = mul(input.normal, (float3x3)mesh.model);
    output.screen_normal = mul(output.world_normal, (float3x3)camera.view);
    output.uv = input.uv;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    float4 color = tex.Sample(tex_sampler, input. uv);
    float3 world_normal = normalize(input.world_normal);
    float3 screen_normal = normalize(input.screen_normal);
    float4 light_color = float4(light.directional_lights[0].color, 1.0);
    color = color * material.diffuse * light_color;

    if (material.spa_mode != 0)
    {
        float2 spa_uv = float2(screen_normal.x * 0.5 + 0.5, 1.0 - (screen_normal.y * 0.5 + 0.5));
        float4 spa_color = spa.Sample(spa_sampler, spa_uv);

        if (material.spa_mode == 1) {
            color *= float4(spa_color.rgb, 1.0);
        }
        else if (material.spa_mode == 2)
            color += float4(spa_color.rgb, 0.0);
    }

    float3 light_direction = normalize(light.directional_lights[0].direction);
    if (material.toon_mode != 0)
    {
        float c = 0.0;
        c = max(0.0, dot(world_normal, -light_direction));
        // c *= shadow_factor(i, pin.camera_depth, pin.shadow_position[i]);
        c = 1.0 - c;

        color *= toon.Sample(toon_sampler, float2(0.0, c));
    }

    return color;
}