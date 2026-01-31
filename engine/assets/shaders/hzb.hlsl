#include "common.hlsli"

struct constant_data
{
    uint prev_buffer;
    uint next_buffer;
    uint prev_width;
    uint prev_height;
    uint next_width;
    uint next_height;
    uint level;
    uint hzb_sampler;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float> prev_buffer = ResourceDescriptorHeap[constant.prev_buffer];
    RWTexture2D<float> next_buffer = ResourceDescriptorHeap[constant.next_buffer];

    if (dtid.x >= constant.next_width || dtid.y >= constant.next_height)
    {
        return;
    }

    if (constant.level == 0)
    {
        next_buffer[dtid.xy] = prev_buffer[dtid.xy];
        return;
    }

    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];
    
    float2 texcoord = (float2(dtid.xy) * 2.0) / float2(constant.prev_width, constant.prev_height);
    float depth = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0, int2(1, 1));

    if ((constant.prev_width & 1) && dtid.x == constant.next_width - 1)
    {
        float right_depth = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0, int2(2, 1));
        depth = min(depth, right_depth);
    }

    if ((constant.prev_height & 1) && dtid.y == constant.next_height - 1)
    {
        float bottom_depth = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0, int2(1, 2));
        depth = min(depth, bottom_depth);
    }

    // TODO: Sampling may be incomplete for bottom-right pixel when both prev_width and prev_height are odd, may need to sample (2,2) offset.

    next_buffer[dtid.xy] = depth;
}