#include "violet_common.hlsli"

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
    float3 uvw : TEXCOORD;
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
        0, 2, 1,
        2, 0, 3,
        4, 1, 5,
        1, 4, 0,
        7, 5, 6,
        5, 7, 4,
        3, 6, 2,
        6, 3, 7,
        4, 3, 0,
        3, 4, 7,
        1, 6, 5,
        6, 1, 2}; // x+, y+, x-, y-, z+, z-*/

    float3 position = camera.position + vertices[indices[input.vertex_id]];

    vs_out result;
    result.position = mul(camera.view_projection, float4(position, 1.0f));
    result.position.z = result.position.w * 0.99999;
    result.uvw = normalize(vertices[indices[input.vertex_id]]);

    return result;
}

float4 fs_main(vs_out input) : SV_TARGET
{
    return sky_texture.Sample(sky_sampler, input.uvw);
}