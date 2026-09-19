#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    clipmap clipmaps[SDF_CLIPMAP_LEVEL_COUNT];

    uint clipmap_state;

    uint invalidated_grids;

    uint mesh_buffer;

    uint clipmap_level_meshes;
    uint clipmap_level_mesh_counts;

    uint invalidated_grid_meshes;
};
PushConstant(constant_data, constant);

groupshared uint gs_mesh_count;

[shader("compute")]
[numthreads(256, 1, 1)]
void cs_main(uint3 gid : SV_GroupID, uint group_index : SV_GroupIndex)
{
#ifdef CALCULATE_GRID_MESH_LIST_OFFSET
    RWStructuredBuffer<uint2> invalidated_grids = ResourceDescriptorHeap[constant.invalidated_grids];
#else
    StructuredBuffer<uint2> invalidated_grids = ResourceDescriptorHeap[constant.invalidated_grids];
#endif

    clipmap_grid grid = clipmap_grid::unpack(invalidated_grids[gid.x]);

    StructuredBuffer<mesh_sdf> meshes = ResourceDescriptorHeap[constant.mesh_buffer];

    StructuredBuffer<uint> clipmap_level_meshes = ResourceDescriptorHeap[constant.clipmap_level_meshes];
    StructuredBuffer<uint> clipmap_level_mesh_counts = ResourceDescriptorHeap[constant.clipmap_level_mesh_counts];

    uint mesh_count = clipmap_level_mesh_counts[grid.level];

    clipmap clipmap = constant.clipmaps[grid.level];

    float grid_extent = clipmap.extent / SDF_CLIPMAP_GRID_COUNT_PER_AXIS;

    float3 grid_min = clipmap.position + grid.coord * grid_extent;
    float3 grid_max = grid_min + grid_extent;

    if (group_index == 0)
    {
        gs_mesh_count = 0;
    }
    GroupMemoryBarrierWithGroupSync();

#ifndef CALCULATE_GRID_MESH_LIST_OFFSET
    RWStructuredBuffer<uint> invalidated_grid_meshes = ResourceDescriptorHeap[constant.invalidated_grid_meshes];
#endif

    uint clipmap_level_meshes_offset = grid.level * SDF_MAX_MESH_COUNT;
    for (uint i = 0; i < mesh_count; i += 256)
    {
        uint mesh_index = i + group_index;
        if (mesh_index < mesh_count)
        {
            uint mesh_id = clipmap_level_meshes[mesh_index + clipmap_level_meshes_offset];
            mesh_sdf mesh = meshes[mesh_id];

            if (intersect_aabb(mesh.volume_bounds_min, mesh.volume_bounds_max, grid_min, grid_max))
            {
#ifdef CALCULATE_GRID_MESH_LIST_OFFSET
                InterlockedAdd(gs_mesh_count, 1);
#else
                uint offset;
                InterlockedAdd(gs_mesh_count, 1, offset);
                invalidated_grid_meshes[grid.mesh_offset + offset] = mesh_id;
#endif
            }
        }
    }

#ifdef CALCULATE_GRID_MESH_LIST_OFFSET
    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0 && gs_mesh_count != 0)
    {
        RWStructuredBuffer<clipmap_state> clipmap_state = ResourceDescriptorHeap[constant.clipmap_state];

        InterlockedAdd(clipmap_state[0].invalidated_grid_mesh_count, gs_mesh_count, grid.mesh_offset);
        grid.mesh_count = gs_mesh_count;

        invalidated_grids[gid.x] = grid.pack();
    }
#endif
}