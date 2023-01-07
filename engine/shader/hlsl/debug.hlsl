#include "violet_mvp.hlsl"

ConstantBuffer<violet_camera> camera : register(b0, space0);

struct vs_in
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    result.position = mul(float4(vin.position, 1.0f), camera.transform_vp);
    result.color = vin.color;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return float4(pin.color, 1.0f);
}