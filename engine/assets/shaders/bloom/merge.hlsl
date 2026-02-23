#include "common.hlsli"

struct constant_data
{
    uint render_target;
    uint bloom;
    float intensity;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];

    uint width;
    uint height;
    render_target.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    Texture2D<float3> bloom = ResourceDescriptorHeap[constant.bloom];

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);
    render_target[dtid.xy] += float4(bloom.SampleLevel(linear_clamp_sampler, texcoord, 0.0) * constant.intensity, 0.0);
}