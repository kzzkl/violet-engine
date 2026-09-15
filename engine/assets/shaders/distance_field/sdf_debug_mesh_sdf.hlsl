#include "common.hlsli"
#include "utils.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint mesh_index;
    uint mesh_buffer;
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

    float2 uv = get_compute_uv(dtid.xy, width, height);

    debug_output[dtid.xy] = float4(1.0, 0.0, 0.0, 1.0);
}