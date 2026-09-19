#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    clipmap clipmaps[SDF_CLIPMAP_LEVEL_COUNT];

    uint clipmap_state;

    uint invalidated_grids;
    uint invalidated_pages;
    uint invalidated_grid_meshes;

    uint mesh_buffer;

    uint page_table;
    uint free_pages;

    uint pages_to_allocate;
    uint pages_to_allocate_indirect_args;

    uint pages_to_update;
    uint pages_to_update_indirect_args;

    uint page_atlas_capacity;
};
PushConstant(constant_data, constant);

groupshared uint gs_intersect_count;

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 gid : SV_GroupID, uint group_index : SV_GroupIndex)
{
    StructuredBuffer<uint> invalidated_pages = ResourceDescriptorHeap[constant.invalidated_pages];
    StructuredBuffer<uint2> invalidated_grids = ResourceDescriptorHeap[constant.invalidated_grids];

    clipmap_page page = clipmap_page::unpack(invalidated_pages[gid.x]);
    clipmap_grid grid = clipmap_grid::unpack(invalidated_grids[page.grid_index]);

    RWStructuredBuffer<uint> invalidated_grid_meshes = ResourceDescriptorHeap[constant.invalidated_grid_meshes];
    StructuredBuffer<mesh_sdf> meshes = ResourceDescriptorHeap[constant.mesh_buffer];

    clipmap clipmap = constant.clipmaps[page.level];
    float page_extent = clipmap.extent / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;

    float3 page_min = clipmap.position + page.coord * page_extent;
    float3 page_max = page_min + page_extent;

    if (group_index == 0)
    {
        gs_intersect_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    uint intersect_count = 0;
    for (uint i = 0; i < grid.mesh_count; i += 64)
    {
        uint mesh_offset = i + group_index;
        if (mesh_offset >= grid.mesh_count)
        {
            break;
        }

        uint mesh_id = invalidated_grid_meshes[grid.mesh_offset + mesh_offset];

        mesh_sdf mesh = meshes[mesh_id];
        if (intersect_aabb(mesh.volume_bounds_min, mesh.volume_bounds_max, page_min, page_max))
        {
            // TODO: sample sdf
            ++intersect_count;
        }
    }

    InterlockedAdd(gs_intersect_count, intersect_count);

    GroupMemoryBarrierWithGroupSync();

    RWTexture3D<uint> page_table = ResourceDescriptorHeap[constant.page_table];

    if (group_index == 0)
    {
        RWStructuredBuffer<clipmap_state> clipmap_state = ResourceDescriptorHeap[constant.clipmap_state];

        uint3 page_table_coord = page.get_page_table_coord();
        clipmap_page_table_entry page_table_entry = clipmap_page_table_entry::unpack(page_table[page_table_coord]);

        if (gs_intersect_count > 0)
        {
            if (page_table_entry.resident())
            {
                uint pages_to_update_offset;
                InterlockedAdd(clipmap_state[0].pages_to_update_count, 1, pages_to_update_offset);

                RWStructuredBuffer<dispatch_command> pages_to_update_indirect_args = ResourceDescriptorHeap[constant.pages_to_update_indirect_args];
                uint dispatch_count = get_dispatch_group_count(pages_to_update_offset, 1, 64);
                if (dispatch_count > 0)
                {
                    InterlockedAdd(pages_to_update_indirect_args[0].x, dispatch_count);
                }
            }
            else
            {
                uint pages_to_allocate_offset;
                InterlockedAdd(clipmap_state[0].pages_to_allocate_count, 1, pages_to_allocate_offset);

                if (pages_to_allocate_offset < constant.page_atlas_capacity)
                {
                    RWStructuredBuffer<uint> pages_to_allocate = ResourceDescriptorHeap[constant.pages_to_allocate];
                    pages_to_allocate[pages_to_allocate_offset] = page.pack();

                    RWStructuredBuffer<dispatch_command> pages_to_allocate_indirect_args = ResourceDescriptorHeap[constant.pages_to_allocate_indirect_args];
                    if (pages_to_allocate_offset % 64 == 0)
                    {
                        InterlockedAdd(pages_to_allocate_indirect_args[0].x, 1);
                    }
                }
                else
                {
                    page_table[page_table_coord] = SDF_INVALID_PAGE_TABLE_ENTRY;
                }
            }
        }
        else
        {
            if (page_table_entry.resident())
            {
                RWStructuredBuffer<uint> free_pages = ResourceDescriptorHeap[constant.free_pages];

                uint free_index;
                InterlockedAdd(clipmap_state[0].free_page_atlas_count, 1, free_index);
                free_pages[free_index] = page_table_entry.page_atlas_id;

                page_table[page_table_coord] = SDF_INVALID_PAGE_TABLE_ENTRY;
            }
        }
    }
}