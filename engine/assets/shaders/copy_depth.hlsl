#include "common.hlsli"

struct constant_data
{
    uint src;
    uint dst;
    uint width;
    uint height;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x < constant.width && dtid.y < constant.height)
    {
        Texture2D<float> src_buffer = ResourceDescriptorHeap[constant.src];
        RWTexture2D<float> dst_buffer = ResourceDescriptorHeap[constant.dst];
        dst_buffer[dtid.xy] = src_buffer[dtid.xy];
    }
}