#include "violet_mesh.hlsl"
#include "violet_camera.hlsl"

struct vs_in
{
    float3 position: POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float2 uv: UV;
};

ConstantBuffer<violet_mesh> mesh : register(b0, space0);
ConstantBuffer<violet_camera> camera : register(b0, space1);

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.position = mul(mesh.model, float4(input.position, 1.0));
    output.position = mul(camera.view_projection, output.position);
    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(1.0, 0.0, 0.0, 1.0);
}