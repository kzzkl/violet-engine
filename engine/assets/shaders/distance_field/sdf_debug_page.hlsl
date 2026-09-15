#include "common.hlsli"
#include "utils.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint debug_output;
    uint depth_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

static const float3 level_colors[8] =
{
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(1.0, 1.0, 0.0),
    float3(1.0, 0.0, 1.0),
    float3(0.0, 1.0, 1.0),
    float3(1.0, 0.5, 0.0),
    float3(0.5, 0.0, 1.0),
};

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    Texture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];
    
    uint width;
    uint height;
    depth_buffer.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    float depth = depth_buffer[dtid.xy];
    if (depth == 0.0)
    {
        return;
    }

    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];

    float2 uv = get_compute_uv(dtid.xy, width, height);
    float3 position_ws = reconstruct_position(depth, uv, camera.matrix_vp_inv).xyz;

    uint clipmap_level = get_clipmap_level(position_ws, camera.position);
    if (clipmap_level == 0xFFFFFFFF)
    {
        debug_output[dtid.xy] = float4(level_colors[7], 1.0);
    }
    else
    {
        float page_extent = ldexp(SDF_CLIPMAP_PAGE_EXTENT, clipmap_level);

        int3 page_coord = floor(position_ws / page_extent);
        int page_id =
            page_coord.x +
            page_coord.y * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS +
            page_coord.z * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS +
            clipmap_level * SDF_CLIPMAP_PAGE_COUNT;

        debug_output[dtid.xy] = float4(lerp(level_colors[clipmap_level], to_color(page_id), 0.5), 1.0);
    }
}