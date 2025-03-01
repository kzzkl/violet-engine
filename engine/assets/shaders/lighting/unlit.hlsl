#include "common.hlsli"

struct constant_data
{
    uint albedo;
    uint padding0;
    uint padding1;
    uint padding2;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);

float4 fs_main(float2 texcoord : TEXCOORD) : SV_TARGET
{
    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[constant.albedo];
    SamplerState point_clamp_sampler = get_point_clamp_sampler();

    return gbuffer_albbedo.Sample(point_clamp_sampler, texcoord);
}