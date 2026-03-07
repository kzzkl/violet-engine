#include "common.hlsli"

struct constant_data
{
    float2 bloom_texel_size;
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
    float2 offset = constant.bloom_texel_size * 0.5;

    float3 bloom_color = 0.0;
    bloom_color += bloom.SampleLevel(linear_clamp_sampler, texcoord + offset, 0.0);
    bloom_color += bloom.SampleLevel(linear_clamp_sampler, texcoord + float2(offset.x, -offset.y), 0.0);
    bloom_color += bloom.SampleLevel(linear_clamp_sampler, texcoord + float2(-offset.x, offset.y), 0.0);
    bloom_color += bloom.SampleLevel(linear_clamp_sampler, texcoord - offset, 0.0);
    bloom_color *= 0.25;

    render_target[dtid.xy] += float4(bloom_color * constant.intensity, 0.0);
}