#ifndef VSM_CULL_HLSLI
#define VSM_CULL_HLSLI

#include "virtual_shadow_map/vsm_common.hlsli"
#include "utils.hlsli"

bool vsm_cull(
    uint vsm_id,
    uint4 page_bounds,
    float4 sphere_vs,
    bool is_static,
    vsm_data vsm,
    StructuredBuffer<uint> virtual_page_table
#ifdef USE_OCCLUSION
    ,
    StructuredBuffer<uint4> physical_page_table,
    Texture2D<float> hzb,
    SamplerState hzb_sampler
#endif
    )
{
    float4 projected_aabb;
    project_shpere_orthographic(sphere_vs, vsm.matrix_p[0][0], vsm.matrix_p[1][1], projected_aabb);

    projected_aabb = projected_aabb * 0.5 + 0.5;

    float4 projected_page_bounds = projected_aabb * VIRTUAL_PAGE_TABLE_SIZE;
    uint4 overlap_page_bounds;
    overlap_page_bounds.xy = max(0.0, floor(projected_page_bounds.xy));
    overlap_page_bounds.zw = max(0.0, floor(projected_page_bounds.zw));

    projected_aabb *= VIRTUAL_RESOLUTION;

    overlap_page_bounds.xy = max(overlap_page_bounds.xy, page_bounds.xy);
    overlap_page_bounds.zw = min(overlap_page_bounds.zw, page_bounds.zw);

    if (overlap_page_bounds.x > overlap_page_bounds.z || overlap_page_bounds.y > overlap_page_bounds.w)
    {
        return false;
    }

    float depth = (sphere_vs.z - sphere_vs.w) * vsm.matrix_p[2][2] + vsm.matrix_p[2][3];

    uint vsm_base = vsm_id * VIRTUAL_PAGE_TABLE_PAGE_COUNT;
    for (uint y = overlap_page_bounds.y; y <= overlap_page_bounds.w; ++y)
    {
        uint row_base = vsm_base + y * VIRTUAL_PAGE_TABLE_SIZE;
        for (uint x = overlap_page_bounds.x; x <= overlap_page_bounds.z; ++x)
        {
            vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[row_base + x]);

            if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_VISIBLE) == 0)
            {
                continue;
            }

            if (is_static && (virtual_page.flags & VIRTUAL_PAGE_FLAG_RESIDENT) != 0)
            {
                continue;
            }

            float2 virtual_texel_offset = float2(x, y) * PAGE_RESOLUTION;

            float4 page_aabb;
            page_aabb.xy = max(virtual_texel_offset, projected_aabb.xy);
            page_aabb.zw = min(virtual_texel_offset + PAGE_RESOLUTION, projected_aabb.zw);

            float width = page_aabb.z - page_aabb.x;
            float height = page_aabb.w - page_aabb.y;

            if (width < 1.0 || height < 1.0)
            {
                continue;
            }

#ifdef USE_OCCLUSION
            uint physical_page_index = get_physical_page_index(virtual_page.physical_page_coord);
            vsm_physical_page physical_page = vsm_physical_page::unpack(physical_page_table[physical_page_index]);

            if (physical_page.flags & PHYSICAL_PAGE_FLAG_HZB_DIRTY)
            {
                return true;
            }

            float2 physical_texel_offset = virtual_page.physical_page_coord * PAGE_RESOLUTION;

            page_aabb.xy = page_aabb.xy - virtual_texel_offset + physical_texel_offset;
            page_aabb.zw = page_aabb.zw - virtual_texel_offset + physical_texel_offset;

            float level = clamp(ceil(log2(max(width, height))), 0.0, 5.0);
            bool visiable = hzb.SampleLevel(hzb_sampler, (page_aabb.xy + page_aabb.zw) * 0.5 / PHYSICAL_RESOLUTION, level) < depth;

            if (visiable)
            {
                return true;
            }
#else
            return true;
#endif
        }
    }

    return false;
}

void get_shadow_draw_offset(bool is_static, uint shadow_batch, out uint command_offset, out uint count_offset)
{
    static const uint MAX_SHADOW_DRAWS_PER_BATCH = 1024 * 20;
    static const uint SHADOW_BATCH_COUNT = 6;

    static const uint STATIC_DRAW_COMMAND_OFFSET = 0;
    static const uint STATIC_DRAW_COUNT_OFFSET = 0;

    static const uint DYNAMIC_DRAW_COMMAND_OFFSET = MAX_SHADOW_DRAWS_PER_BATCH * SHADOW_BATCH_COUNT;
    static const uint DYNAMIC_DRAW_COUNT_OFFSET = SHADOW_BATCH_COUNT;

    command_offset = is_static ? STATIC_DRAW_COMMAND_OFFSET : DYNAMIC_DRAW_COMMAND_OFFSET;
    command_offset += shadow_batch * MAX_SHADOW_DRAWS_PER_BATCH;
    count_offset = is_static ? STATIC_DRAW_COUNT_OFFSET : DYNAMIC_DRAW_COUNT_OFFSET;
    count_offset += shadow_batch;
}

#endif