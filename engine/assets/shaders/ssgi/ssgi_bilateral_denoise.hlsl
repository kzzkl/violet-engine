#include "common.hlsli"
#include "gbuffer.hlsli"

struct constant_data
{
    uint src;
    uint dst;
    float2 texel_size;
    uint normal_buffer;
};
PushConstant(constant_data, constant);

static const float BLUR_RADIUS = 8.0;
static const float BLUR_SIGMA = BLUR_RADIUS * 0.5;
static const float BLUR_FALLOFF = 1.0 / (2.0 * BLUR_SIGMA * BLUR_SIGMA);

struct bilateral_data
{
    float3 normal;
    float depth;
};

float gaussian_weight(float radius, float sigma)
{
    float a = radius / sigma;
    return exp(-a * a);
}

float bilateral_weight(bilateral_data center, bilateral_data tap)
{
    float depth_weight = max(0.0, 1.0 - abs(center.depth - tap.depth));
    float normal_weight = max(0.0, dot(center.normal, tap.normal));
    normal_weight = normal_weight * normal_weight;
    normal_weight = normal_weight * normal_weight;

    return depth_weight * normal_weight;
}

void bilateral_blur(
    bilateral_data center,
    float2 texcoord,
    float2 texcoord_delta,
    Texture2D<float4> src,
    SamplerState sampler,
    Texture2D<uint> normal_buffer,
    uint2 normal_buffer_extent,
    inout float3 total_color,
    inout float total_weight)
{
    float r = 1.0;

    for (; r < BLUR_RADIUS * 0.5; r += 1.0)
    {
        float4 sample_value = src.SampleLevel(sampler, texcoord + r * texcoord_delta, 0.0);

        bilateral_data tap;
        tap.depth = sample_value.a;
        tap.normal = unpack_gbuffer_normal(normal_buffer, (texcoord + r * texcoord_delta) * normal_buffer_extent);

        float weight = sample_value.a < 0.0 ? 0.0 : gaussian_weight(r, BLUR_SIGMA) * bilateral_weight(center, tap);

        total_color += sample_value.rgb * weight;
        total_weight += weight;
    }

    for (; r < BLUR_RADIUS; r += 2.0)
    {
        float4 sample_value = src.SampleLevel(sampler, texcoord + r * texcoord_delta, 0.0);

        bilateral_data tap;
        tap.depth = sample_value.a;
        tap.normal = unpack_gbuffer_normal(normal_buffer, (texcoord + r * texcoord_delta) * normal_buffer_extent);

        float weight = sample_value.a < 0.0 ? 0.0 : gaussian_weight(r, BLUR_SIGMA) * bilateral_weight(center, tap);

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
    
    bilateral_data center;
    center.depth = src_value.a;

    Texture2D<uint> normal_buffer = ResourceDescriptorHeap[constant.normal_buffer];
    uint2 normal_buffer_extent;
    normal_buffer.GetDimensions(normal_buffer_extent.x, normal_buffer_extent.y);

    center.normal = unpack_gbuffer_normal(normal_buffer, texcoord * normal_buffer_extent);

    float3 total_color = src_value.rgb;
    float total_weight = 1.0;

#ifdef BLUR_HORIZONTAL
    float2 texcoord_delta = float2(constant.texel_size.x, 0.0);
#else
    float2 texcoord_delta = float2(0.0, constant.texel_size.y);
#endif

    bilateral_blur(center, texcoord, texcoord_delta, src, point_clamp_sampler, normal_buffer, normal_buffer_extent, total_color, total_weight);
    bilateral_blur(center, texcoord, -texcoord_delta, src, point_clamp_sampler, normal_buffer, normal_buffer_extent, total_color, total_weight);

    dst[dtid.xy] = float4(total_color / max(total_weight, 1e-6), src_value.a);
}