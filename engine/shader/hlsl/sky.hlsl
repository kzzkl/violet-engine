#include "violet_mvp.hlsl"

ConstantBuffer<violet_camera> camera : register(b0, space0);

TextureCube sky_texture : register(t0, space1);
SamplerState sky_sampler : register(s2);

struct vs_in
{
    uint vertex_id : SV_VertexID;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 uvw : UVW;
};

vs_out vs_main(vs_in vin)
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

    float3 position = camera.position + vertices[indices[vin.vertex_id]];

    vs_out result;
    result.position = mul(float4(position, 1.0f), camera.transform_vp).xyww;
    result.uvw = vertices[indices[vin.vertex_id]];
    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return sky_texture.Sample(sky_sampler, pin.uvw);
}