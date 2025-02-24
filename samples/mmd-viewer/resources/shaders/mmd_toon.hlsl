#include "common.hlsli"

struct gbuffer_data
{
    uint color;
    uint ao_buffer;
};
ConstantBuffer<gbuffer_data> constant : register(b0, space1);

float4 fs_main(float2 texcoord : TEXCOORD) : SV_TARGET
{
    SamplerState point_clamp_sampler = get_point_clamp_sampler();
    Texture2D<float4> gbuffer_color = ResourceDescriptorHeap[constant.color];

    float4 color = gbuffer_color.Sample(point_clamp_sampler, texcoord);

#ifdef USE_AO_BUFFER
    Texture2D<float> ao_buffer = ResourceDescriptorHeap[constant.ao_buffer];
    float ao = ao_buffer.Sample(point_clamp_sampler, texcoord);
    color.rgb *= ao;
#endif

    return color;
}