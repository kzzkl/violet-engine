#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint clipmap_state;

    uint clipmap_levels;
    uint clipmap_level_offset;

    uint invalidation_regions;
    uint invalidation_region_count;

    uint invalidated_grids;
    uint invalidated_grid_indirect_args;
    uint invalidated_pages;
    uint invalidated_page_indirect_args;
};
PushConstant(constant_data, constant);

struct invalidation_region
{
    float3 bounding_box_min;
    uint padding0;
    float3 bounding_box_max;
    uint padding1;
};

static const uint MAX_INVALIDATION_BOUNDS = 256; 

groupshared uint gs_intersect_invalidation_bounds[MAX_INVALIDATION_BOUNDS];
groupshared uint gs_intersect_count;

groupshared uint gs_dirty_page_offset;
groupshared uint gs_dirty_page_count;

groupshared uint gs_grid_index;

[shader("compute")]
[numthreads(4, 4, 4)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    StructuredBuffer<invalidation_region> invalidation_regions = ResourceDescriptorHeap[constant.invalidation_regions];

    uint level = dtid.z / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;

    StructuredBuffer<clipmap_level> clipmap_levels = ResourceDescriptorHeap[constant.clipmap_levels];
    clipmap_level clipmap_level = clipmap_levels[constant.clipmap_level_offset + level];

    uint3 grid_coord = gid;
    grid_coord.z %= SDF_CLIPMAP_GRID_COUNT_PER_AXIS;

    float grid_extent = clipmap_level.extent / SDF_CLIPMAP_GRID_COUNT_PER_AXIS;
    float3 grid_min = clipmap_level.position + grid_coord * grid_extent;
    float3 grid_max = grid_min + grid_extent;

    if (group_index == 0)
    {
        gs_intersect_count = 0;
        gs_dirty_page_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint i = 0; i < constant.invalidation_region_count; i += 64)
    {
        uint invalidation_region_index = i + group_index;
        
        bool intersect = invalidation_region_index < constant.invalidation_region_count;
        if (intersect)
        {
            invalidation_region region = invalidation_regions[invalidation_region_index];
            intersect = intersect_aabb(region.bounding_box_min, region.bounding_box_max, grid_min, grid_max);
        }

        if (intersect)
        {
            uint offset;
            InterlockedAdd(gs_intersect_count, 1, offset);

            if (offset < MAX_INVALIDATION_BOUNDS)
            {
                gs_intersect_invalidation_bounds[offset] = invalidation_region_index;
            }
            else
            {
                break;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    float page_extent = clipmap_level.extent / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
    float3 page_min = grid_min + gtid * page_extent;
    float3 page_max = page_min + page_extent;

    uint dirty_page_offset = 0xFFFFFFFF;

    uint intersect_count = min(gs_intersect_count, MAX_INVALIDATION_BOUNDS);
    for (uint i = 0; i < intersect_count; ++i)
    {
        invalidation_region region = invalidation_regions[gs_intersect_invalidation_bounds[i]];
        if (intersect_aabb(region.bounding_box_min, region.bounding_box_max, page_min, page_max))
        {
            InterlockedAdd(gs_dirty_page_count, 1, dirty_page_offset);
            break;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0 && gs_dirty_page_count != 0)
    {
        RWStructuredBuffer<clipmap_state> clipmap_state = ResourceDescriptorHeap[constant.clipmap_state];

        RWStructuredBuffer<uint2> invalidated_grids = ResourceDescriptorHeap[constant.invalidated_grids];
        RWStructuredBuffer<dispatch_command> invalidated_grid_indirect_args = ResourceDescriptorHeap[constant.invalidated_grid_indirect_args];

        InterlockedAdd(clipmap_state[0].invalidated_grid_count, 1, gs_grid_index);
        InterlockedAdd(invalidated_grid_indirect_args[0].x, 1);

        clipmap_grid grid;
        grid.coord = grid_coord;
        grid.level = level;
        grid.mesh_offset = 0;
        grid.mesh_count = 0;
        invalidated_grids[gs_grid_index] = grid.pack();

        InterlockedAdd(clipmap_state[0].invalidated_page_count, gs_dirty_page_count, gs_dirty_page_offset);

        RWStructuredBuffer<dispatch_command> invalidated_page_indirect_args = ResourceDescriptorHeap[constant.invalidated_page_indirect_args];
        InterlockedAdd(invalidated_page_indirect_args[0].x, gs_dirty_page_count);
    }

    GroupMemoryBarrierWithGroupSync();

    if (dirty_page_offset != 0xFFFFFFFF)
    {
        RWStructuredBuffer<uint> invalidated_pages = ResourceDescriptorHeap[constant.invalidated_pages];

        clipmap_page page;
        page.coord = grid_coord * SDF_CLIPMAP_PAGES_PER_GRID_AXIS + gtid;
        page.level = level;
        page.grid_index = gs_grid_index;
        invalidated_pages[dirty_page_offset + gs_dirty_page_offset] = page.pack();
    }
}