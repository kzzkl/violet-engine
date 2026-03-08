#include "common.hlsli"

struct constant_data
{
    uint render_target;
    uint depth_buffer;
    uint aerial_perspective_lut;
    float distance_per_slice;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];

    uint width;
    uint height;
    render_target.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float2 uv = get_compute_texcoord(dtid.xy, width, height);

    Texture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];
    float depth = depth_buffer.SampleLevel(get_point_clamp_sampler(), uv, 0.0);
    if (depth == 0.0)
    {
        return;
    }

    float4 position_ws = reconstruct_position(depth, uv, camera.matrix_vp_inv);
    float distance = length(position_ws.xyz - camera.position);

    Texture3D<float4> aerial_perspective_lut = ResourceDescriptorHeap[constant.aerial_perspective_lut];

    float slice = distance / constant.distance_per_slice;
    if (slice < 0.5)
    {
        slice = 0.5;
    }
    float w = sqrt(slice / 32.0);

    float4 aerial_perspective = aerial_perspective_lut.SampleLevel(get_linear_clamp_sampler(), float3(uv, w), 0.0);
    float3 in_scatter = aerial_perspective.xyz;
    float transmittance = aerial_perspective.w;

    float3 color = render_target[dtid.xy].xyz;
    render_target[dtid.xy] = float4(color * transmittance + in_scatter, 1.0);
}