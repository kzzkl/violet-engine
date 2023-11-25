#include "violet_node.hlsl"

struct basic_material
{
    float3 color;
};

ConstantBuffer<violet_node> node : register(b0, space0);
ConstantBuffer<basic_material> material : register(b0, space1);
ConstantBuffer<violet_camera> camera : register(b0, space2);

struct vs_in
{
    float3 position : POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    result.position = mul(float4(vin.position, 1.0), camera.transform_vp);

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return float4(material.color, 1.0);
}