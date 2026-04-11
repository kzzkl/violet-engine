#include "common.hlsli"

struct constant_data
{
    uint2 size;
    float2 size_inv;
    uint render_target;
    uint blue_noise;
    float2 blue_noise_size_inv;
    uint blue_noise_slice;
};
PushConstant(constant_data, constant);

static const float NOISE_GRANULARITY = 0.5 / 255.0;

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= constant.size.x || dtid.y >= constant.size.y)
    {
        return;
    }

    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];

    SamplerState point_repeat_sampler = get_point_repeat_sampler();
    Texture2DArray<float> blue_noise = ResourceDescriptorHeap[constant.blue_noise];

    float noise = blue_noise.SampleLevel(point_repeat_sampler, float3(dtid.xy * constant.blue_noise_size_inv, constant.blue_noise_slice), 0);
    noise = lerp(-NOISE_GRANULARITY, NOISE_GRANULARITY, noise);

    render_target[dtid.xy] += noise;
}