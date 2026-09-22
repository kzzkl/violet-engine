#include "distance_field/sdf_common.hlsli"
#include "common.hlsli"

struct constant_data
{
    uint clipmap_levels;
    uint clipmap_level_offset;

    uint mesh_buffer;
    uint mesh_count;

    uint clipmap_level_meshes;
    uint clipmap_level_mesh_counts;
};
PushConstant(constant_data, constant);

groupshared uint gs_mesh_counts[SDF_CLIPMAP_LEVEL_COUNT];
groupshared uint gs_mesh_offsets[SDF_CLIPMAP_LEVEL_COUNT];

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    StructuredBuffer<mesh_sdf> meshes = ResourceDescriptorHeap[constant.mesh_buffer];

    if (group_index < SDF_CLIPMAP_LEVEL_COUNT)
    {
        gs_mesh_counts[group_index] = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint offsets[SDF_CLIPMAP_LEVEL_COUNT];

    StructuredBuffer<clipmap_level> clipmap_levels = ResourceDescriptorHeap[constant.clipmap_levels];

    if (dtid.x < constant.mesh_count)
    {
        mesh_sdf mesh = meshes[dtid.x];

        for (uint level = 0; level < SDF_CLIPMAP_LEVEL_COUNT; ++level)
        {
            clipmap_level clipmap_level = clipmap_levels[constant.clipmap_level_offset + level];

            if (intersect_aabb(
                mesh.volume_bounds_min,
                mesh.volume_bounds_max,
                clipmap_level.position,
                clipmap_level.position + clipmap_level.extent))
            {
                InterlockedAdd(gs_mesh_counts[level], 1, offsets[level]);
            }
            else
            {
                offsets[level] = 0xFFFFFFFF;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    RWStructuredBuffer<uint> clipmap_level_meshes = ResourceDescriptorHeap[constant.clipmap_level_meshes];
    RWStructuredBuffer<uint> clipmap_level_mesh_counts = ResourceDescriptorHeap[constant.clipmap_level_mesh_counts];

    if (group_index < SDF_CLIPMAP_LEVEL_COUNT)
    {
        if (gs_mesh_counts[group_index] != 0)
        {
            InterlockedAdd(clipmap_level_mesh_counts[group_index], gs_mesh_counts[group_index], gs_mesh_offsets[group_index]);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (dtid.x < constant.mesh_count)
    {
        for (uint level = 0; level < SDF_CLIPMAP_LEVEL_COUNT; ++level)
        {
            if (offsets[level] != 0xFFFFFFFF)
            {
                clipmap_level_meshes[gs_mesh_offsets[level] + offsets[level] + level * SDF_MAX_MESH_COUNT] = dtid.x;
            }
        }
    }
}