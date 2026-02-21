#include "common.hlsli"

struct constant_data
{
    uint src;
    uint dst;
    float threshold;
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

    Texture2D<float4> src = ResourceDescriptorHeap[constant.src];

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    uint src_width;
    uint src_height;
    src.GetDimensions(src_width, src_height);
    float2 texcoord = get_compute_texcoord(dtid.xy * 2.0, src_width, src_height);

    float3 color0 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, uint2(0, 0)).xyz;
    float3 color1 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, uint2(1, 0)).xyz;
    float3 color2 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, uint2(0, 1)).xyz;
    float3 color3 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, uint2(1, 1)).xyz;

    float3 color = (color0 + color1 + color2 + color3) * 0.25;
    dst[dtid.xy] = luminance(color) > constant.threshold ? color : 0.0;
}