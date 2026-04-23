#include "common.hlsli"

ConstantBuffer<camera_data> camera : register(b0, space1);

struct constant_data
{
    uint depth_buffer;
    uint motion_vector;
};
PushConstant(constant_data, constant);

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float2> motion_vector_buffer = ResourceDescriptorHeap[constant.motion_vector];

    uint width;
    uint height;
    motion_vector_buffer.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float2 uv = get_compute_texcoord(dtid.xy, width, height);

    Texture2D<float> buffer = ResourceDescriptorHeap[constant.depth_buffer];
    float depth = buffer.SampleLevel(get_point_clamp_sampler(), uv, 0.0);

    float4 position_ws;
    if (depth == 0.0)
    {
        uv.y = 1.0 - uv.y;
        float4 ndc = float4(uv * 2.0 - 1.0, 0.0, 1.0);
        position_ws = mul(camera.matrix_vp_inv, ndc);
        position_ws = float4(normalize(position_ws.xyz), 0.0);
    }
    else
    {
        position_ws = reconstruct_position(depth, uv, camera.matrix_vp_inv);
    }

    float4 curr_position_cs = mul(camera.matrix_vp_no_jitter, position_ws);
    curr_position_cs.xy /= curr_position_cs.w;

    float4 prev_position_cs = mul(camera.prev_matrix_vp_no_jitter, position_ws);
    prev_position_cs.xy /= prev_position_cs.w;

    float2 motion_vector = prev_position_cs.xy - curr_position_cs.xy;
    motion_vector *= float2(0.5, -0.5);

    motion_vector_buffer[dtid.xy] = motion_vector;
}