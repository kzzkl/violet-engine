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

static const uint SDF_TILE_COUNT_PER_PAGE_AXIS = 2;
static const uint SDF_VOXEL_COUNT_PER_TILE_AXIS = SDF_CLIPMAP_PAGE_RESOLUTION / SDF_TILE_COUNT_PER_PAGE_AXIS;

uint3 get_coord(uint index, uint dim)
{
    uint3 coord;
    coord.x = index % dim;
    index /= dim;

    coord.y = index % dim;
    coord.z = index / dim;

    return coord;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 gid : SV_GroupID, uint3 gtid : SV_GroupThreadID, uint group_index : SV_GroupIndex)
{
    uint page_index = gid.x / 8;
    uint tile_index = gid.x % 8;

    uint3 tile_coord = get_coord(tile_index, SDF_TILE_COUNT_PER_PAGE_AXIS);
    uint3 voxel_coord = get_coord(gtid.x, SDF_VOXEL_COUNT_PER_TILE_AXIS);

    StructuredBuffer<uint> pages_to_update = ResourceDescriptorHeap[constant.pages_to_update];
    clipmap_page page = clipmap_page::unpack(pages_to_update[page_index]);

    StructuredBuffer<clipmap_level> clipmap_levels = ResourceDescriptorHeap[constant.clipmap_levels];
    clipmap_level clipmap_level = clipmap_levels[constant.clipmap_level_offset + page.level];

    float page_extent = clipmap_level.extent / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
    float voxel_extent = page_extent / SDF_CLIPMAP_UNIQUE_PAGE_RESOLUTION;

    float3 page_min = clipmap_level.position + page.coord * page_extent;
    float3 tile_min = page_min + tile_coord * SDF_VOXEL_COUNT_PER_TILE_AXIS * voxel_extent;
    float3 tile_max = tile_min + SDF_VOXEL_COUNT_PER_TILE_AXIS * voxel_extent;

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

    float3 position_ws = tile_min + voxel_coord * voxel_extent;

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
        float3 half_extent = distance_field.volume_extent * 0.5;

        float3 to_box = (abs(position) - half_extent) * mesh.volume_to_world_scale.xyz;
        float distance_to_box = length(max(0.0, to_box)) + min(0.0, max3(to_box));

        if (distance_to_box > min_distance)
        {
            continue;
        }

        float3 clamped = clamp(position, -half_extent, half_extent);
        float distance_to_mesh = distance_field.sample(clamped, brick_table, brick_atlas, linear_clamp_sampler) * mesh.volume_to_world_scale.w;
        float distance = distance_to_mesh + max(0.0, distance_to_box);
        distance = max(distance, distance_to_box);

        min_distance = min(min_distance, distance);
    }

    Texture3D<uint> page_table = ResourceDescriptorHeap[constant.page_table];
    uint3 page_table_coord = page.get_page_table_coord();

    clipmap_page_table_entry page_table_entry = clipmap_page_table_entry::unpack(page_table[page_table_coord]);
    if (!page_table_entry.resident())
    {
        return;
    }

    uint3 page_atlas_coord = page_table_entry.get_page_atlas_coord();
    RWTexture3D<float> page_atlas = ResourceDescriptorHeap[constant.page_atlas];

    uint3 atlas_coord = page_atlas_coord * SDF_CLIPMAP_PAGE_RESOLUTION;
    atlas_coord += tile_coord * SDF_VOXEL_COUNT_PER_TILE_AXIS;
    atlas_coord += voxel_coord;
    page_atlas[atlas_coord] = encode_distance(min_distance, clipmap_level.max_distance);
}