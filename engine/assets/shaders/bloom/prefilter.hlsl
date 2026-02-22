#include "common.hlsli"

struct constant_data
{
    float3 curve; // (threshold - knee, knee * 2, 0.25 / knee)
    float threshold;
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

    float3 color0 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, int2(-1, -1)).xyz;
    float3 color1 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, int2(1, -1)).xyz;
    float3 color2 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, int2(1, 1)).xyz;
    float3 color3 = src.SampleLevel(linear_clamp_sampler, texcoord, 0.0, int2(-1, 1)).xyz;
    float3 color = (color0 + color1 + color2 + color3) * 0.25;

    float brightness = max(color.r, max(color.g, color.b));

    float soft = brightness - constant.curve.x;
    soft = clamp(soft, 0.0, constant.curve.y);
    soft = soft * soft * constant.curve.z;

    float contribution = max(soft, brightness - constant.threshold) / max(brightness, 0.00001);

    dst[dtid.xy] = color * contribution;
}