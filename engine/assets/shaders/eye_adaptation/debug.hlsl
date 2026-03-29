#include "common.hlsli"

struct constant_data
{
    uint render_target;
    uint histogram;
    uint debug_output;
};
PushConstant(constant_data, constant);

static const uint HISTOGRAM_BIN_COUNT = 64;

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    Texture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];
    StructuredBuffer<uint> histogram = ResourceDescriptorHeap[constant.histogram];

    uint width;
    uint height;
    debug_output.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float4 color = render_target[dtid.xy];

    float max_histogram_value = 0.0;
    for (uint i = 0; i < HISTOGRAM_BIN_COUNT; ++i)
    {
        max_histogram_value = max(max_histogram_value, histogram[i]);
    }
    max_histogram_value = 1.0 / max_histogram_value;

    uint histogram_width = width * 0.25;
    uint histogram_height = height * 0.25;
    if (dtid.x < histogram_width && dtid.y > height - histogram_height)
    {
        color.xyz *= 0.5;

        uint histogram_index = 64 * dtid.x / histogram_width;
        float bin_value = histogram[histogram_index] * max_histogram_value;

        if (dtid.y > height - histogram_height * bin_value)
        {
            color = float4(1.0, 1.0, 1.0, 1.0);
        }
    }

    debug_output[dtid.xy] = color;
}