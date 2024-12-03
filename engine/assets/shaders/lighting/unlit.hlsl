#include "common.hlsli"
#include "gbuffer.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);

struct gbuffer_data
{
    uint albedo;
    uint depth;
    uint padding0;
    uint padding1;
};
ConstantBuffer<gbuffer_data> gbuffer : register(b0, space2);

float4 fs_main(float2 uv : TEXCOORD) : SV_TARGET
{
    SamplerState gbuffer_sampler = SamplerDescriptorHeap[scene.point_sampler];

    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[gbuffer.albedo];

    gbuffer_packed data;
    data.albedo = gbuffer_albbedo.Sample(gbuffer_sampler, uv);

    float3 albedo;
    gbuffer_decode(data, albedo);

    return float4(albedo, 1.0);
}