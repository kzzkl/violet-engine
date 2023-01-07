#include "violet_mvp.hlsl"

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


ConstantBuffer<violet_camera> camera : register(b0, space2);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float edge : EDGE;
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in vin)
{
    float4 position = mul(mul(float4(vin.position, 1.0f), object.transform_m), camera.transform_vp);
    float3 normal = mul(mul(vin.normal, (float3x3)object.transform_m), (float3x3)camera.transform_v);
    float2 screen_normal = normalize(normal.xy);

    position.xy += screen_normal * 0.003f * edge_size * vin.edge * position.w;

    vs_out result;
    result.position = position;
    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return edge_color;
}