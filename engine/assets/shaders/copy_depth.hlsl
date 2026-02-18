#include "common.hlsli"

struct constant_data
{
    uint src;
    uint dst;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float> src_buffer = ResourceDescriptorHeap[constant.src];
    RWTexture2D<float> dst_buffer = ResourceDescriptorHeap[constant.dst];

    uint width;
    uint height;
    src_buffer.GetDimensions(width, height);

    if (dtid.x < width && dtid.y < height)
    {
        dst_buffer[dtid.xy] = src_buffer[dtid.xy];
    }
}