#include "common.hlsli"

struct constant_data
{
    uint prev_buffer;
    uint next_buffer;
    uint level;
    uint width;
    uint height;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float> prev_buffer = ResourceDescriptorHeap[constant.prev_buffer];
    RWTexture2D<float> next_buffer = ResourceDescriptorHeap[constant.next_buffer];

    if (constant.level == 0)
    {
        next_buffer[dtid.xy] = prev_buffer[dtid.xy];
    }
    else
    {
        float2 texcoord = float2(dtid.xy)  / float2(constant.width, constant.height);
        float4 depths = prev_buffer.Gather(get_linear_clamp_sampler(), texcoord, int2(1, 1));
        next_buffer[dtid.xy] = max(max(depths.x, depths.y), max(depths.z, depths.w));
    }
}