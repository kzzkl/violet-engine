#include "distance_field/sdf_common.hlsli"
#include "common.hlsli"

struct constant_data
{
    clipmap clipmaps[SDF_CLIPMAP_LEVEL_COUNT];

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

    if (dtid.x < constant.mesh_count)
    {
        mesh_sdf mesh = meshes[dtid.x];

        for (uint level = 0; level < SDF_CLIPMAP_LEVEL_COUNT; ++level)
        {
            clipmap clipmap = constant.clipmaps[level];

            if (intersect_aabb(
                mesh.bounding_box_min,
                mesh.bounding_box_max,
                clipmap.position,
                clipmap.position + clipmap.extent))
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