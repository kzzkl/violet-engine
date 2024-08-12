#include "violet_common.hlsli"

ConstantBuffer<violet_mesh> mesh : register(b0, space0);

struct mmd_material
{
    float4 edge_color;
    float edge_size;
};

ConstantBuffer<mmd_material> material : register(b0, space1);
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

vs_out vs_main(vs_in input)
{
    float4 position = mul(camera.view_projection, mul(mesh.model, float4(input.position, 1.0)));
    float3 normal = mul((float3x3)camera.view, mul((float3x3)mesh.model, input.normal));
    float2 screen_normal = normalize(normal.xy);

    position.xy += screen_normal * 0.003f * material.edge_size * input.edge * position.w;

    vs_out output;
    output.position = position;
    return output;
}

float4 fs_main(vs_out input) : SV_TARGET
{
    return material.edge_color;
}