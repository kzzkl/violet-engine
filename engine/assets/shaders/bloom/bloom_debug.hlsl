#include "common.hlsli"

struct constant_data
{
    uint src;
    uint debug_output;
    float intensity;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void debug_prefilter(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    uint width;
    uint height;
    debug_output.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    Texture2D<float3> prefilter = ResourceDescriptorHeap[constant.src];

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);
    debug_output[dtid.xy] = float4(prefilter.SampleLevel(get_linear_repeat_sampler(), texcoord, 0), 1.0);
}

[numthreads(8, 8, 1)]
void debug_bloom(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    uint width;
    uint height;
    debug_output.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    Texture2D<float3> bloom = ResourceDescriptorHeap[constant.src];

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);
    debug_output[dtid.xy] = float4(bloom.SampleLevel(get_linear_repeat_sampler(), texcoord, 0.0) * 0.125 * constant.intensity, 1.0);
}
