#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint clipmap_levels;
    uint clipmap_level_offset;

    uint page_table;
    uint page_atlas;

    uint debug_output;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    uint width;
    uint height;
    debug_output.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }
}