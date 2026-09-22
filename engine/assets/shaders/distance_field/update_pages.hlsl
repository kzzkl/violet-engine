#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint clipmap_levels;
    uint clipmap_level_offset;

    uint invalidated_grids;
    uint invalidated_grid_meshes;

    uint mesh_buffer;

    uint page_table;
    uint page_atlas;

    uint pages_to_update;

    uint distance_field_buffer;
    uint brick_table;
    uint brick_atlas;
};
PushConstant(constant_data, constant);

groupshared uint gs_tile_mesh_count;
groupshared uint gs_tile_meshes[512];

[shader("compute")]
[numthreads(4, 4, 4)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint3 gtid : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    uint page_index = dtid.x / 8;
    uint3 tile_coord = (dtid / 4) % 2;

    StructuredBuffer<uint> pages_to_update = ResourceDescriptorHeap[constant.pages_to_update];
    clipmap_page page = clipmap_page::unpack(pages_to_update[page_index]);

    StructuredBuffer<clipmap_level> clipmap_levels = ResourceDescriptorHeap[constant.clipmap_levels];
    clipmap_level clipmap_level = clipmap_levels[constant.clipmap_level_offset + page.level];

    float page_extent = clipmap_level.extent / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
    float tile_extent = page_extent / 2;

    float3 page_min = clipmap_level.position + page.coord * page_extent;
    float3 tile_min = page_min + tile_coord * tile_extent;
    float3 tile_max = tile_min + tile_extent;

    StructuredBuffer<uint> invalidated_grid_meshes = ResourceDescriptorHeap[constant.invalidated_grid_meshes];
    StructuredBuffer<mesh_sdf> meshes = ResourceDescriptorHeap[constant.mesh_buffer];

    StructuredBuffer<uint2> invalidated_grids = ResourceDescriptorHeap[constant.invalidated_grids];
    clipmap_grid grid = clipmap_grid::unpack(invalidated_grids[page.grid_index]);

    if (group_index == 0)
    {
        gs_tile_mesh_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    for (uint i = 0; i < grid.mesh_count; i += 64)
    {
        uint mesh_offset = i + group_index;
        if (mesh_offset >= grid.mesh_count)
        {
            break;
        }

        uint mesh_id = invalidated_grid_meshes[grid.mesh_offset + mesh_offset];

        mesh_sdf mesh = meshes[mesh_id];
        if (intersect_aabb(mesh.volume_bounds_min, mesh.volume_bounds_max, tile_min, tile_max))
        {
            uint mesh_index;
            InterlockedAdd(gs_tile_mesh_count, 1, mesh_index);

            if (mesh_index < 512)
            {
                gs_tile_meshes[mesh_index] = mesh_id;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        gs_tile_mesh_count = min(gs_tile_mesh_count, 512);
    }

    GroupMemoryBarrierWithGroupSync();

    float3 position_ws = tile_min + gtid * (page_extent / SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION);

    StructuredBuffer<distance_field> distance_fields = ResourceDescriptorHeap[constant.distance_field_buffer];
    StructuredBuffer<uint> brick_table = ResourceDescriptorHeap[constant.brick_table];
    Texture3D<float> brick_atlas = ResourceDescriptorHeap[constant.brick_atlas];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float min_distance = clipmap_level.max_distance;
    for (uint i = 0; i < gs_tile_mesh_count; ++i)
    {
        mesh_sdf mesh = meshes[gs_tile_meshes[i]];
        distance_field distance_field = distance_fields[mesh.distance_field_id];

        float3 position = mul(mesh.world_to_volume, float4(position_ws, 1.0)).xyz;
        float distance = get_volume_bounds_distance(position, distance_field.volume_extent);
        if (distance > min_distance)
        {
            continue;
        }

        distance = max(0.0, distance);
        distance += distance_field.sample(position, brick_table, brick_atlas, linear_clamp_sampler);

        min_distance = min(min_distance, distance);
    }

    Texture3D<uint> page_table = ResourceDescriptorHeap[constant.page_table];
    uint3 page_table_coord = page.get_page_table_coord();

    clipmap_page_table_entry page_table_entry = clipmap_page_table_entry::unpack(page_table[page_table_coord]);
    uint3 page_atlas_coord = page_table_entry.get_page_atlas_coord();

    RWTexture3D<float> page_atlas = ResourceDescriptorHeap[constant.page_atlas];

    page_atlas[page_atlas_coord * SDF_CLIPMAP_PAGE_RESOLUTION + tile_coord * 4 + gtid] = encode_distance(min_distance, clipmap_level.max_distance);
}