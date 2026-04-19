#include "common.hlsli"

struct constant_data
{
    uint src;
    uint dst;
    float2 texel_size;
    float blur_factor;
};
PushConstant(constant_data, constant);

static const float BLUR_RADIUS = 8.0;
static const float BLUR_SIGMA = BLUR_RADIUS * 0.5;
static const float BLUR_FALLOFF = 1.0 / (2.0 * BLUR_SIGMA * BLUR_SIGMA);

void bilatral_blur(
    float4 src_value,
    float2 texcoord,
    float2 texcoord_delta,
    Texture2D<float4> src,
    SamplerState sampler,
    inout float3 total_color,
    inout float total_weight)
{
    float r = 1.0;

    for (; r < BLUR_RADIUS * 0.5; r += 1.0)
    {
        float4 sample_value = src.SampleLevel(sampler, texcoord + r * texcoord_delta, 0.0);

        float dz = (src_value.a - sample_value.a) * constant.blur_factor;
        float weight = sample_value.a < 0.0 ? 0.0 : exp(-r * r * BLUR_FALLOFF - dz * dz);

        total_color += sample_value.rgb * weight;
        total_weight += weight;
    }

    for (; r < BLUR_RADIUS; r += 2.0)
    {
        float4 sample_value = src.SampleLevel(sampler, texcoord + r * texcoord_delta, 0.0);

        float dz = (src_value.a - sample_value.a) * constant.blur_factor;
        float weight = sample_value.a < 0.0 ? 0.0 : exp(-r * r * BLUR_FALLOFF - dz * dz);

        total_color += sample_value.rgb * weight;
        total_weight += weight;
    }
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float4> src = ResourceDescriptorHeap[constant.src];

    uint width;
    uint height;
    src.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    RWTexture2D<float4> dst = ResourceDescriptorHeap[constant.dst];
    SamplerState point_clamp_sampler = get_point_clamp_sampler();

    float2 texcoord = get_compute_texcoord(dtid.xy, width, height);

    float4 src_value = src.SampleLevel(point_clamp_sampler, texcoord, 0.0);
    if (src_value.a < 0.0)
    {
        dst[dtid.xy] = src_value;
        return;
    }

    float3 total_color = src_value.rgb;
    float total_weight = 1.0;

#ifdef BLUR_HORIZONTAL
    float2 texcoord_delta = float2(constant.texel_size.x, 0.0);
#else
    float2 texcoord_delta = float2(0.0, constant.texel_size.y);
#endif

    bilatral_blur(src_value, texcoord, texcoord_delta, src, point_clamp_sampler, total_color, total_weight);
    bilatral_blur(src_value, texcoord, -texcoord_delta, src, point_clamp_sampler, total_color, total_weight);

    dst[dtid.xy] = float4(total_color / max(total_weight, 1e-6), src_value.a);
}