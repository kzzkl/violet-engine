#include "violet_mesh.hlsl"
#include "violet_camera.hlsl"
#include "violet_light.hlsl"

struct basic_material
{
    float3 color;
};

ConstantBuffer<violet_mesh> mesh : register(b0, space0);
ConstantBuffer<basic_material> material : register(b0, space1);
ConstantBuffer<violet_camera> camera : register(b0, space2);
ConstantBuffer<violet_light> light : register(b0, space3);

struct vs_in
{
    float3 position : POSITION;
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.position = mul(camera.view_projection, mul(mesh.model, float4(input.position, 1.0)));

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(1.0, 0.0, 0.0, 1.0);
}