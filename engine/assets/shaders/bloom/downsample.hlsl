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
    RWTexture2D<float3> dst = ResourceDescriptorHeap[constant.dst];

    uint width;
    uint height;
    dst.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    Texture2D<float3> src = ResourceDescriptorHeap[constant.src];

    float3 color0 = src.Load(uint3(dtid.xy * 2 + uint2(0, 0), 0));
    float3 color1 = src.Load(uint3(dtid.xy * 2 + uint2(1, 0), 0));
    float3 color2 = src.Load(uint3(dtid.xy * 2 + uint2(0, 1), 0));
    float3 color3 = src.Load(uint3(dtid.xy * 2 + uint2(1, 1), 0));

    dst[dtid.xy] = (color0 + color1 + color2 + color3) * 0.25;
}