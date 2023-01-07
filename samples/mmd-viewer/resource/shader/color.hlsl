#include "violet_mvp.hlsl"
#include "violet_light.hlsl"

ConstantBuffer<violet_object> object : register(b0, space0);

cbuffer mmd_material : register(b0, space1)
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
Texture2D tex : register(t0, space1);
Texture2D toon : register(t1, space1);
Texture2D spa : register(t2, space1);
SamplerState sampler_clamp : register(s1);

ConstantBuffer<violet_camera> camera : register(b0, space2);
ConstantBuffer<violet_light> light : register(b0, space3);
Texture2D<float> shadow_map[16] : register(t0, space3);
SamplerComparisonState shadow_sampler : register(s6);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_normal : WORLD_NORMAL;
    float3 screen_normal : SCREEN_NORMAL;
    float2 uv : UV;

    float4 shadow_position[VIOLET_MAX_SHADOW_COUNT] : SHADOW_POSITION;
    float camera_depth : CAMERA_DEPTH;
};

vs_out vs_main(vs_in vin)
{
    float4 world_positon = mul(float4(vin.position, 1.0f), object.transform_m);

    vs_out result;
    result.position = mul(world_positon, camera.transform_vp);
    result.world_normal = mul(vin.normal, (float3x3)object.transform_m);
    result.screen_normal = mul(result.world_normal, (float3x3)camera.transform_v);
    result.uv = vin.uv;

    for (uint i = 0 ; i < light.shadow_count; ++i)
        result.shadow_position[i] = mul(world_positon, light.shadow_v[i]);
    result.camera_depth = mul(world_positon, camera.transform_v).z;

    return result;
}

float shadow_factor(uint light_index, float camera_depth, float4 shadow_position)
{
    uint cascade_index = shadow_cascade_index(camera_depth, light);

    shadow_position.xyz /= shadow_position.w;
    shadow_position *= light.cascade_scale[light_index][cascade_index];
    shadow_position += light.cascade_offset[light_index][cascade_index];
    
    return shadow_pcf(shadow_map[light_index * 4 + cascade_index], shadow_sampler, shadow_position);
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    float4 color = tex.Sample(sampler_clamp, pin. uv);
    float3 world_normal = normalize(pin.world_normal);
    float3 screen_normal = normalize(pin.screen_normal);
    color = color * diffuse;

    if (spa_mode != 0)
    {
        float2 spa_uv = float2(screen_normal.x * 0.5f + 0.5f, 1.0f - (screen_normal.y * 0.5f + 0.5f));
        float3 spa_color = spa.Sample(sampler_clamp, spa_uv).rgb;

        if (spa_mode == 1)
            color *= float4(spa_color, 1.0f);
        else if (spa_mode == 2)
            color += float4(spa_color, 0.0f);
    }

    if (toon_mode != 0)
    {
        float c = 0.0f;
        for (uint i = 0; i < light.directional_light_count; ++i)
        {
            c = max(0.0f, dot(world_normal, -light.directional_light[i].direction));
            c *= shadow_factor(i, pin.camera_depth, pin.shadow_position[i]);
            c = 1.0f - c;
        }

        color *= toon.Sample(sampler_clamp, float2(0.0f, c));
    }

    return color;
}