#include "violet_common.hlsli"

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

vs_out vs_main(vs_in input)
{
    vs_out output;

    output.position = mul(camera.view_projection, float4(input.position, 1.0f));
    output.color = input.color;

    return output;
}

float4 fs_main(vs_out input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}