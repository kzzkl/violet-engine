cbuffer ash_object : register(b0)
{
    float4x4 transform_m;
    float4x4 transform_mv;
    float4x4 transform_mvp;
};

cbuffer mmd_material : register(b1)
{
    float4 diffuse;
    float3 specular;
    float specular_strength;
    uint toon_mode;
    uint spa_mode;
};

cbuffer mmd_skeleton : register(b2)
{
    float4x4 offset[512];
};

cbuffer ash_pass : register(b3)
{
    float4 camera_position;
    float4 camera_direction;

    float4x4 transform_v;
    float4x4 transform_p;
    float4x4 transform_vp;
};

Texture2D tex : register(t0);
Texture2D toon : register(t1);
Texture2D spa : register(t2);

SamplerState sampler_wrap : register(s0);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
    uint4 bone : BONE;
    float3 weight : BONE_WEIGHT;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    float weights[4] = { 0.0, 0.0, 0.0, 0.0 };
    weights[0] = vin.weight.x;
    weights[1] = vin.weight.y;
    weights[2] = vin.weight.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    result.position = float4(0.0, 0.0, 0.0, 1.0);
    result.normal = float3(0.0, 0.0, 0.0);

    float4 position = {0.0, 0.0, 0.0, 0.0};
    float4 normal = {0.0, 0.0, 0.0, 0.0};

    for (int i = 0; i < 4; ++i)
    {
        position += weights[i] * mul(float4(vin.position, 1.0f), offset[vin.bone[i]]);
        normal += weights[i] * mul(float4(vin.normal, 1.0f), offset[vin.bone[i]]);
    }
    position.w = 1.0f;
    normal.w = 0.0f;

    result.position = mul(position, transform_mvp);
    result.uv = vin.uv;
    result.normal = mul(normal, transform_mv).xyz;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    float4 color = tex.Sample(sampler_wrap, pin. uv);
    //clip(color.w < 0.1f ? -1 : 1);

    pin.normal = normalize(pin.normal);
    color = color * diffuse;

    if (spa_mode != 0)
    {
        float2 spa_uv = {pin.normal.x * 0.5f + 0.5f, 1.0f - (pin.normal.y * 0.5f + 0.5f)};
        float3 spa_color = spa.Sample(sampler_wrap, spa_uv).rgb;

        if (spa_mode == 1)
            color *= float4(spa_color, 1.0f);
        else if (spa_mode == 2)
            color += float4(spa_color, 0.0f);
    }

    if (toon_mode != 0)
    {
        float3 light = {1.0f, 1.0f, 1.0f};
        float3 light_dir = normalize(float3(1.0f, -1.0f, 1.0f));

        float c = dot(pin.normal, light_dir);
        c = clamp(c + 0.5f, 0.0f, 1.0f);
        
        color *= toon.Sample(sampler_wrap, float2(0.0f, c));
    }

    return color;
}