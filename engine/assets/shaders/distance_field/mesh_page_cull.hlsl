#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint clipmap_state;

    uint clipmap_levels;
    uint clipmap_level_offset;

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

    uint distance_field_buffer;
    uint brick_table;
    uint brick_atlas;
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

    StructuredBuffer<uint> invalidated_grid_meshes = ResourceDescriptorHeap[constant.invalidated_grid_meshes];
    StructuredBuffer<mesh_sdf> meshes = ResourceDescriptorHeap[constant.mesh_buffer];

    StructuredBuffer<clipmap_level> clipmap_levels = ResourceDescriptorHeap[constant.clipmap_levels];
    clipmap_level clipmap_level = clipmap_levels[constant.clipmap_level_offset + page.level];

    float page_extent = clipmap_level.extent / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
    float3 page_min = clipmap_level.position + page.coord * page_extent;
    float3 page_max = page_min + page_extent;
    float3 page_center = (page_min + page_max) * 0.5;

    if (group_index == 0)
    {
        gs_intersect_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    StructuredBuffer<distance_field> distance_fields = ResourceDescriptorHeap[constant.distance_field_buffer];
    StructuredBuffer<uint> brick_table = ResourceDescriptorHeap[constant.brick_table];
    Texture3D<float> brick_atlas = ResourceDescriptorHeap[constant.brick_atlas];

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
            distance_field distance_field = distance_fields[mesh.distance_field_id];

            float3 center = mul(mesh.world_to_volume, float4(page_center, 1.0)).xyz;
            float3 half_extent = distance_field.volume_extent * 0.5;
            float3 to_box = (abs(center) - half_extent) * mesh.volume_to_world_scale.xyz;

            float distance = length(max(0.0, to_box)) + min(0.0, max3(to_box));
            if (distance < page_extent)
            {
                // TODO: sample sdf
                ++intersect_count;
            }
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
            bool need_update = true;

            if (!page_table_entry.resident())
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
                    need_update = false;
                }
            }

            if (need_update)
            {
                uint pages_to_update_offset;
                InterlockedAdd(clipmap_state[0].pages_to_update_count, 1, pages_to_update_offset);

                RWStructuredBuffer<uint> pages_to_update = ResourceDescriptorHeap[constant.pages_to_update];
                pages_to_update[pages_to_update_offset] = page.pack();

                RWStructuredBuffer<dispatch_command> pages_to_update_indirect_args = ResourceDescriptorHeap[constant.pages_to_update_indirect_args];

                // 8 tiles per page. 64 threads(voxels) per tile.
                InterlockedAdd(pages_to_update_indirect_args[0].x, 8);
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