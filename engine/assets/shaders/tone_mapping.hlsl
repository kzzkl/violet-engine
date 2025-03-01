#include "common.hlsli"

struct constant_data
{
    uint hdr;
    uint ldr;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float4> hdr = ResourceDescriptorHeap[constant.hdr];
    RWTexture2D<float4> ldr = ResourceDescriptorHeap[constant.ldr];

    float3 color = hdr[dtid.xy].rgb;

    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;

    color = saturate((color * (a * color + b)) / (color * (c * color + d) + e));

    ldr[dtid.xy] = float4(color, 1.0);
}