#include "common.hlsli"

struct constant_data
{
    float2 scale_offset;
    uint histogram;
    uint exposure;
    float low_percent;
    float high_percent;
    float min_brightness;
    float max_brightness;
    float speed_down;
    float speed_up;
    float delta_time;
};
PushConstant(constant_data, constants);

static const uint HISTOGRAM_BIN_COUNT = 64;

float get_luminance_from_histogram_bin(uint bin, float2 scale_offset)
{
    float log_luminance = ((float)bin / (HISTOGRAM_BIN_COUNT - 1) - scale_offset.y) / scale_offset.x;
    return exp2(log_luminance);
}

[numthreads(1, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> histogram = ResourceDescriptorHeap[constants.histogram];

    float max_histogram_value = 0.0;
    for (uint i = 0; i < HISTOGRAM_BIN_COUNT; ++i)
    {
        max_histogram_value = max(max_histogram_value, histogram[i]);
    }
    max_histogram_value = 1.0 / max_histogram_value;

    float sum = 0.0;
    for (uint i = 0; i < HISTOGRAM_BIN_COUNT; ++i)
    {
        sum += histogram[i] * max_histogram_value;
    }

    float4 fliter = float4(0.0, 0.0, sum * constants.low_percent, sum * constants.high_percent);

    for (uint i = 0; i < HISTOGRAM_BIN_COUNT; ++i)
    {
        float bin_value = histogram[i] * max_histogram_value;

        float offset = min(bin_value, fliter.z);
        bin_value -= offset;
        fliter.zw -= offset;

        bin_value = min(bin_value, fliter.w);
        fliter.w -= bin_value;

        float luminance = get_luminance_from_histogram_bin(i, constants.scale_offset);
        fliter.xy += float2(bin_value * luminance, bin_value);
    }

    float average_luminance = clamp(fliter.x / max(fliter.y, 1e-6), constants.min_brightness, constants.max_brightness);

    float key_value = 1.03 - (2.0 / (2.0 + log2(average_luminance + 1.0)));
    float curr_exposure = key_value / average_luminance;

    RWTexture2D<float> exposure = ResourceDescriptorHeap[constants.exposure];
    float prev_exposure = exposure[uint2(0, 0)];

    float delta = curr_exposure - prev_exposure;
    float speed = delta < 0.0 ? constants.speed_up : constants.speed_down;

    exposure[uint2(0, 0)] = prev_exposure + delta * (1.0 - exp2(-constants.delta_time * speed));
}