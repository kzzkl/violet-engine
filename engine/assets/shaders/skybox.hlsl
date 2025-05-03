#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 position : SV_POSITION;
    float3 texcoord : TEXCOORD;
};

vs_output vs_main(uint vertex_id : SV_VertexID)
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

    const int indexes[36] = {
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

    float3 position = camera.position + vertices[indexes[vertex_id]] * 100.0;

    vs_output result;
    result.position = mul(camera.matrix_vp_no_jitter, float4(position, 1.0));
    result.position.z = result.position.w * 0.00001;
    result.texcoord = normalize(vertices[indexes[vertex_id]]);

    return result;
}

float4 fs_main(vs_output input) : SV_TARGET
{
    TextureCube<float4> sky_texture = ResourceDescriptorHeap[scene.skybox];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    return sky_texture.Sample(linear_clamp_sampler, normalize(input.texcoord));
}