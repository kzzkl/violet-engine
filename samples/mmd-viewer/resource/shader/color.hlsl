cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
};

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

cbuffer ash_camera : register(b0, space2)
{
    float3 camera_position;
    float3 camera_direction;

    float4x4 transform_v;
    float4x4 transform_p;
    float4x4 transform_vp;
};

struct ash_directional_light_data
{
    float3 direction;
    float _padding_0;
    float3 color;
    float _padding_1;
};

cbuffer ash_light : register(b0, space3)
{
    ash_directional_light_data directional_light[4];
    uint directional_light_count;
};

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_normal : W_NORMAL;
    float3 screen_normal : S_NORMAL;
    float2 uv : UV;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;
    result.position = mul(float4(vin.position, 1.0f), transform_vp);
    result.world_normal = vin.normal;
    result.screen_normal = mul(result.world_normal, (float3x3)transform_v);
    result.uv = vin.uv;

    return result;
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
        for (uint i = 0; i < directional_light_count; ++i)
        {
            c = dot(world_normal, normalize(-directional_light[0].direction));
        }
        c = clamp(c + 0.5f, 0.0f, 1.0f);

        color *= toon.Sample(sampler_clamp, float2(0.0f, c));
    }

    return color;
}