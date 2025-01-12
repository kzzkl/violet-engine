#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
struct gbuffer_data
{
    uint albedo;
    uint padding0;
    uint padding1;
    uint padding2;
};
ConstantBuffer<gbuffer_data> constant : register(b0, space2);

float4 fs_main(float2 texcoord : TEXCOORD) : SV_TARGET
{
    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[constant.albedo];
    SamplerState point_clamp_sampler = get_point_clamp_sampler();

    return gbuffer_albbedo.Sample(point_clamp_sampler, texcoord);
}