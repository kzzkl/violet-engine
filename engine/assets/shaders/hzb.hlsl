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
    Texture2D<float> prev_buffer = ResourceDescriptorHeap[constant.prev_buffer];
    RWTexture2D<float> next_buffer = ResourceDescriptorHeap[constant.next_buffer];

    uint prev_width;
    uint prev_height;
    prev_buffer.GetDimensions(prev_width, prev_height);

    uint next_width;
    uint next_height;
    next_buffer.GetDimensions(next_width, next_height);

    if (dtid.x >= next_width || dtid.y >= next_height)
    {
        return;
    }

    if (constant.level == 0)
    {
        next_buffer[dtid.xy] = prev_buffer[dtid.xy];
        return;
    }

    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];
    
    float2 texcoord = (float2(dtid.xy) * 2.0) / float2(prev_width, prev_height);
    float depth = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0, int2(1, 1));

    bool right_edge = (prev_width & 1) && dtid.x == next_width - 1;
    bool bottom_edge = (prev_height & 1) && dtid.y == next_height - 1;

    if (right_edge && bottom_edge)
    {
        float bottom_right_depth = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0, int2(2, 2));
        depth = min(depth, bottom_right_depth);
    }
    else if (right_edge)
    {
        float right_depth = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0, int2(2, 1));
        depth = min(depth, right_depth);
    }
    else if (bottom_edge)
    {
        float bottom_depth = prev_buffer.SampleLevel(hzb_sampler, texcoord, 0, int2(1, 2));
        depth = min(depth, bottom_depth);
    }

    next_buffer[dtid.xy] = depth;
}