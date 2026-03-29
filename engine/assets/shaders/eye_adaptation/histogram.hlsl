#include "common.hlsli"

struct constant_data
{
    float2 scale_offset;
    uint render_target;
    uint histogram;
};
PushConstant(constant_data, constant);

static const uint HISTOGRAM_BIN_COUNT = 64;

groupshared uint gs_histogram[HISTOGRAM_BIN_COUNT];

float get_histogram_bin_from_luminance(float luminance, float2 scale_offset)
{
    return saturate(log2(luminance) * scale_offset.x + scale_offset.y);
}

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID)
{
    Texture2D<float3> render_target = ResourceDescriptorHeap[constant.render_target];

    uint group_thread_index = gtid.y * 8 + gtid.x;
    if (group_thread_index < HISTOGRAM_BIN_COUNT)
    {
        gs_histogram[group_thread_index] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint height;
    uint width;
    render_target.GetDimensions(width, height);

    if (dtid.x < width && dtid.y < height)
    {
        float3 color = render_target[dtid.xy];
        float luminance = get_luminance(color);
        uint bin = (uint)(get_histogram_bin_from_luminance(luminance, constant.scale_offset) * (HISTOGRAM_BIN_COUNT - 1));

        uint weight = 1;
        InterlockedAdd(gs_histogram[bin], weight);
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_thread_index < HISTOGRAM_BIN_COUNT)
    {
        RWStructuredBuffer<uint> histogram = ResourceDescriptorHeap[constant.histogram];
        InterlockedAdd(histogram[group_thread_index], gs_histogram[group_thread_index]);
    }
}