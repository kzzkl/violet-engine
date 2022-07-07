[[vk::binding(0, 0)]]
cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
};

[[vk::binding(0, 1)]]
cbuffer mmd_material : register(b0, space1)
{
    float4 diffuse;
    float3 specular;
    float specular_strength;
    uint toon_mode;
    uint spa_mode;
};

[[vk::binding(1, 1)]]
Texture2D tex : register(t0, space1);

[[vk::binding(2, 1)]]
Texture2D toon : register(t1, space1);

[[vk::binding(3, 1)]]
Texture2D spa : register(t2, space1);
SamplerState sampler_wrap : register(s0);

[[vk::binding(0, 2)]]
cbuffer ash_camera : register(b0, space2)
{
    float3 camera_position;
    float3 camera_direction;

    float4x4 transform_v;
    float4x4 transform_p;
    float4x4 transform_vp;
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
    float3 normal : NORMAL;
    float2 uv : UV;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;
    result.position = mul(mul(float4(vin.position, 1.0f), transform_m), transform_vp);
    result.normal = mul(mul(float4(vin.normal, 0.0f), transform_m), transform_v).xyz;
    result.uv = vin.uv;

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

    // return float4(1.0f, 1.0f, 1.0f, 1.0f);
    return color;
}