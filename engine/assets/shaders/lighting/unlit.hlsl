#include "common.hlsli"
#include "gbuffer.hlsli"

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
    SamplerState point_repeat_sampler = SamplerDescriptorHeap[scene.point_repeat_sampler];

    gbuffer::packed gbuffer_packed;
    gbuffer_packed.albedo = gbuffer_albbedo.Sample(point_repeat_sampler, texcoord);

    gbuffer::data gbuffer = gbuffer::unpack(gbuffer_packed);

    return float4(gbuffer.albedo, 1.0);
}