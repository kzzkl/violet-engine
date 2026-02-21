#include "common.hlsli"

struct constant_data
{
    uint src;
    uint dst;
    float texel_size;
};
PushConstant(constant_data, constant);

static const float weights[3] = {0.22702702, 0.31621621, 0.07027027};
static const float offsets[3] = {0.0, 1.38461538, 3.23076923};

[numthreads(8, 8, 1)]
void blur_horizontal(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float3> src = ResourceDescriptorHeap[constant.src];

    uint width;
    uint height;
    src.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);

    float3 color = 0.0;
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x - offsets[2] * constant.texel_size, texcoord.y), 0.0) * weights[2];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x - offsets[1] * constant.texel_size, texcoord.y), 0.0) * weights[1];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x + offsets[0] * constant.texel_size, texcoord.y), 0.0) * weights[0];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x + offsets[1] * constant.texel_size, texcoord.y), 0.0) * weights[1];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x + offsets[2] * constant.texel_size, texcoord.y), 0.0) * weights[2];

    RWTexture2D<float3> dst = ResourceDescriptorHeap[constant.dst];
    dst[dtid.xy] = color;
}

[numthreads(8, 8, 1)]
void blur_vertical(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float3> src = ResourceDescriptorHeap[constant.src];

    uint width;
    uint height;
    src.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);

    float3 color = 0.0;
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x, texcoord.y - offsets[2] * constant.texel_size), 0.0) * weights[2];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x, texcoord.y - offsets[1] * constant.texel_size), 0.0) * weights[1];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x, texcoord.y + offsets[0] * constant.texel_size), 0.0) * weights[0];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x, texcoord.y + offsets[1] * constant.texel_size), 0.0) * weights[1];
    color += src.SampleLevel(linear_clamp_sampler, float2(texcoord.x, texcoord.y + offsets[2] * constant.texel_size), 0.0) * weights[2];

    RWTexture2D<float3> dst = ResourceDescriptorHeap[constant.dst];
    dst[dtid.xy] = color;
}