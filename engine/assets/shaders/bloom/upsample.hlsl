#include "common.hlsli"

struct constant_data
{
    float2 prev_texel_size;
    uint prev_src;
    uint curr_src;
    uint dst;
    float radius;
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

    Texture2D<float3> prev_src = ResourceDescriptorHeap[constant.prev_src];
    Texture2D<float3> curr_src = ResourceDescriptorHeap[constant.curr_src];

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float2 offset = 0.5 * constant.prev_texel_size;

    uint prev_width;
    uint prev_height;
    prev_src.GetDimensions(prev_width, prev_height);

    float2 prev_texcoord = get_compute_texcoord(dtid.xy * 0.5, prev_width, prev_height);
    float3 prev_color = 0.0;
    prev_color += prev_src.SampleLevel(linear_clamp_sampler, prev_texcoord + offset, 0.0);
    prev_color += prev_src.SampleLevel(linear_clamp_sampler, prev_texcoord + float2(offset.x, -offset.y), 0.0);
    prev_color += prev_src.SampleLevel(linear_clamp_sampler, prev_texcoord + float2(-offset.x, offset.y), 0.0);
    prev_color += prev_src.SampleLevel(linear_clamp_sampler, prev_texcoord - offset, 0.0);
    prev_color *= 0.25;

    float2 curr_texcoord = get_compute_texcoord(dtid.xy, width, height);
    float3 curr_color = curr_src.SampleLevel(linear_clamp_sampler, curr_texcoord, 0.0);

    dst[dtid.xy] = curr_color + prev_color * constant.radius;
}