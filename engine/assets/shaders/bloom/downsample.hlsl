#include "common.hlsli"

struct constant_data
{
    float2 texel_size;
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

    Texture2D<float4> src = ResourceDescriptorHeap[constant.src];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);
    float2 offset = 0.5 * constant.texel_size;

    float2 uv[4];
    uv[0] = texcoord + offset * float2(1.0, 1.0);
    uv[1] = texcoord + offset * float2(1.0, -1.0);
    uv[2] = texcoord + offset * float2(-1.0, 1.0);
    uv[3] = texcoord + offset * float2(-1.0, -1.0);

    float3 color0 = src.SampleLevel(linear_clamp_sampler, uv[0], 0.0).xyz;
    float3 color1 = src.SampleLevel(linear_clamp_sampler, uv[1], 0.0).xyz;
    float3 color2 = src.SampleLevel(linear_clamp_sampler, uv[2], 0.0).xyz;
    float3 color3 = src.SampleLevel(linear_clamp_sampler, uv[3], 0.0).xyz;

    dst[dtid.xy] = (color0 + color1 + color2 + color3) * 0.25;
}