#include "hzb_trace.hlsli"

struct constant_data
{
    uint gbuffers[8];
    uint depth_buffer;
    uint hzb;
    float2 hzb_texel_size;
    uint hzb_level_count;
    uint radiance_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> radiance_buffer = ResourceDescriptorHeap[constant.radiance_buffer];

    uint width;
    uint height;
    radiance_buffer.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float2 uv = get_compute_texcoord(dtid.xy, width, height);

    Texture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];

    float depth = depth_buffer[dtid.xy];
    if (depth == 0.0)
    {
        radiance_buffer[dtid.xy] = float4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float3 ray_origin_ws = reconstruct_position(depth, uv, camera.matrix_vp_inv).xyz;
    float3 ray_direction_ws = normalize(float3(1.0, 1.0, 1.0));

    bool hit = hzb_trace(
        ray_origin_ws,
        ray_direction_ws,
        0.01,
        64,
        ResourceDescriptorHeap[constant.hzb],
        constant.hzb_texel_size,
        constant.hzb_level_count,
        camera);

    radiance_buffer[dtid.xy] = float4(hit ? 1.0 : 0.0, 0.0, 0.0, 1.0);
}