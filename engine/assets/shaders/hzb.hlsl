#include "common.hlsli"

struct constant_data
{
    uint prev_buffer;
    uint next_buffer;
    uint level;
    uint hzb_sampler;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float> next_buffer = ResourceDescriptorHeap[constant.next_buffer];

    uint next_width;
    uint next_height;
    next_buffer.GetDimensions(next_width, next_height);

    if (dtid.x >= next_width || dtid.y >= next_height)
    {
        return;
    }

    float2 texcoord = (float2(dtid.xy) + 0.5) / float2(next_width, next_height);

    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

    Texture2D<float> prev_buffer = ResourceDescriptorHeap[constant.prev_buffer];
    next_buffer[dtid.xy] = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0);
}