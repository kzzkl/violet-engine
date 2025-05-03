#include "common.hlsli"

ConstantBuffer<camera_data> camera : register(b0, space1);

struct constant_data
{
    uint depth_buffer;
    uint motion_vector;

    uint width;
    uint height;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= constant.width || dtid.y >= constant.height)
    {
        return;
    }
    
    float2 texcoord = (float2(dtid.xy) + 0.5) / float2(constant.width, constant.height);

    float4 position_ws = reconstruct_position(constant.depth_buffer, texcoord, camera.matrix_vp_inv);

    float4 position_cs = mul(camera.matrix_vp_no_jitter, position_ws);
    position_cs.xy /= position_cs.w;

    float4 prev_position_cs = mul(camera.prev_matrix_vp_no_jitter, position_ws);
    prev_position_cs.xy /= prev_position_cs.w;

    float2 motion_vector = position_cs.xy - prev_position_cs.xy;
    motion_vector.y = -motion_vector.y;
    motion_vector *= 0.5;

    RWTexture2D<float2> motion_vector_buffer = ResourceDescriptorHeap[constant.motion_vector];
    motion_vector_buffer[dtid.xy] = motion_vector;
}