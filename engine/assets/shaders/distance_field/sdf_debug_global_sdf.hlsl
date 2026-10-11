#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"
#include "ray.hlsli"

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

float sample_clipmap(
    float3 position,
    clipmap_level clipmap_level,
    Texture3D<uint> page_table,
    Texture3D<float> page_atlas,
    SamplerState sampler)
{
    float3 coord = (position - clipmap_level.position) / clipmap_level.page_extent;

    uint3 page_coord = floor(coord);
    float3 page_uv = coord - page_coord;

    uint3 page_table_coord = clipmap_level.get_page_table_coord(page_coord);

    clipmap_page_table_entry page_table_entry = clipmap_page_table_entry::unpack(page_table[page_table_coord]);
    if (!page_table_entry.resident())
    {
        return clipmap_level.max_distance;
    }

    uint3 atlas_extent;
    page_atlas.GetDimensions(atlas_extent.x, atlas_extent.y, atlas_extent.z);

    uint3 page_atlas_coord = page_table_entry.get_page_atlas_coord();
    float3 atlas_texel = page_atlas_coord * SDF_CLIPMAP_PAGE_RESOLUTION + page_uv * SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION;
    float3 atlas_uv = (atlas_texel + 0.5) / atlas_extent;
    return decode_distance(page_atlas.SampleLevel(sampler, atlas_uv, 0.0), clipmap_level.max_distance);
}

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

    StructuredBuffer<clipmap_level> clipmap_levels = ResourceDescriptorHeap[constant.clipmap_levels];
    Texture3D<uint> page_table = ResourceDescriptorHeap[constant.page_table];
    Texture3D<float> page_atlas = ResourceDescriptorHeap[constant.page_atlas];

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    clipmap_level clipmap_level = clipmap_levels[constant.clipmap_level_offset];
    float3 clipmap_min = clipmap_level.position;
    float3 clipmap_max = clipmap_level.position + clipmap_level.extent;

    ray ray = ray::create(dtid.xy, width, height, camera.position, camera.matrix_vp_inv);

    bool hit = false;

    float surface_epsilon = 0.1 * clipmap_level.max_distance;

    float t = 0.0;
    for (uint step = 0; step < 64; ++step)
    {
        float3 position = ray.origin + ray.direction * t;

        if (any(position < clipmap_min) || any(position > clipmap_max))
        {
            break;
        }

        float distance = sample_clipmap(position, clipmap_level, page_table, page_atlas, linear_clamp_sampler);

        if (distance < surface_epsilon)
        {
            hit = true;
            break;
        }

        t += distance;
    }

    float4 position_cs = mul(camera.matrix_vp, float4(ray.origin + ray.direction * t, 1.0));
    float depth = position_cs.z / position_cs.w;
    depth = 1.0 - t / 50.0;

    if (hit)
    {
        debug_output[dtid.xy] = float4(depth, depth, depth, 1.0);
    }
    else
    {
        debug_output[dtid.xy] = float4(0.0, 0.0, 0.0, 1.0);
    }
}