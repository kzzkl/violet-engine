#include "violet_camera.hlsl"

ConstantBuffer<violet_camera> camera : register(b0, space0);

TextureCube sky_texture : register(t1, space0);
SamplerState sky_sampler : register(s1, space0);

struct vs_in
{
    uint vertex_id : SV_VertexID;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 uvw : UVW;
};

vs_out vs_main(vs_in input)
{
    const float3 vertices[8] = {
        float3(1.0, 1.0, 1.0),
        float3(1.0, 1.0, -1.0),
        float3(1.0, -1.0, -1.0),
        float3(1.0, -1.0, 1.0),
        float3(-1.0, 1.0, 1.0),
        float3(-1.0, 1.0, -1.0),
        float3(-1.0, -1.0, -1.0),
        float3(-1.0, -1.0, 1.0)};

    const int indices[36] = {
        0, 1, 2,
        2, 3, 0,
        4, 5, 1,
        1, 0, 4,
        7, 6, 5,
        5, 4, 7,
        3, 2, 6,
        6, 7, 3,
        4, 0, 3,
        3, 7, 4,
        1, 5, 6,
        6, 2, 1}; // x+, y+, x-, y-, z+, z-*/

    float3 position = camera.position + vertices[indices[input.vertex_id]];

    vs_out result;
    result.position = mul(camera.view_projection, float4(position, 1.0f));
    result.position.z = result.position.w * 0.99999;
    result.uvw = normalize(vertices[indices[input.vertex_id]]);

    return result;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return sky_texture.Sample(sky_sampler, input.uvw);
}