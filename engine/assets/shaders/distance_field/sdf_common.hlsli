#ifndef SDF_COMMON_HLSLI
#define SDF_COMMON_HLSLI 

static const float SDF_CLIPMAP_EXTENT = 50.0;

static const uint SDF_CLIPMAP_RESOLUTION = 256;
static const uint SDF_CLIPMAP_PAGE_RESOLUTION = 8;

static const uint SDF_CLIPMAP_PAGE_COUNT_PER_AXIS = SDF_CLIPMAP_RESOLUTION / SDF_CLIPMAP_PAGE_RESOLUTION;
static const uint SDF_CLIPMAP_PAGE_COUNT_PER_AXIS_HALF = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS / 2;
static const uint SDF_CLIPMAP_PAGE_COUNT = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;

static const float SDF_CLIPMAP_PAGE_EXTENT = SDF_CLIPMAP_EXTENT / SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;

static const uint SDF_CLIPMAP_PAGES_PER_GRID_AXIS  = 4;

static const uint SDF_CLIPMAP_GRID_COUNT_PER_AXIS = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS / SDF_CLIPMAP_PAGES_PER_GRID_AXIS ;
static const uint SDF_CLIPMAP_GRID_COUNT = SDF_CLIPMAP_GRID_COUNT_PER_AXIS * SDF_CLIPMAP_GRID_COUNT_PER_AXIS * SDF_CLIPMAP_GRID_COUNT_PER_AXIS;

static const uint SDF_CLIPMAP_LEVEL_COUNT = 4;

static const uint SDF_CLIPMAP_PAGE_ATLAS_WIDTH = 1024;
static const uint SDF_CLIPMAP_PAGE_ATLAS_HEIGHT = 1024;
static const float SDF_CLIPMAP_PAGE_ATLAS_OCCUPANCY = 0.3;

static const uint SDF_MAX_MESH_COUNT = 1024 * 16;

static const uint SDF_INVALID_PAGE_TABLE_ENTRY = 0xFFFFFFFF;

struct clipmap_state
{
    uint invalidated_grid_count;
    uint invalidated_grid_mesh_count;
    uint invalidated_page_count;

    uint pages_to_allocate_count;
    uint pages_to_update_count;

    int free_page_atlas_count;
};

struct mesh_sdf
{
    float3 bounding_box_min;
    uint mesh_id;
    float3 bounding_box_max;
    uint sdf_id;
};

struct clipmap
{
    float3 position;
    float extent;
};

struct clipmap_grid
{
    uint3 coord;
    uint level;

    uint mesh_offset;
    uint mesh_count;

    static clipmap_grid unpack(uint2 packed)
    {
        clipmap_grid grid;

        uint local_id = packed.x % SDF_CLIPMAP_GRID_COUNT;

        uint N = SDF_CLIPMAP_GRID_COUNT_PER_AXIS;
        uint NN = N * N;

        uint z = local_id / NN;
        uint rem = local_id - z * NN;

        uint y = rem / N;
        uint x = rem - y * N;

        grid.coord = uint3(x, y, z);

        grid.level = packed.x / SDF_CLIPMAP_GRID_COUNT;

        grid.mesh_offset = packed.y >> 16;
        grid.mesh_count = packed.y & 0xFFFF;
    
        return grid;
    }

    uint2 pack()
    {
        uint2 packed;
        packed.x =
            coord.x +
            coord.y * SDF_CLIPMAP_GRID_COUNT_PER_AXIS +
            coord.z * SDF_CLIPMAP_GRID_COUNT_PER_AXIS * SDF_CLIPMAP_GRID_COUNT_PER_AXIS +
            level * SDF_CLIPMAP_GRID_COUNT;
        packed.y = (mesh_offset << 16) | mesh_count;
        return packed;
    }
};

struct clipmap_page
{
    uint3 coord;
    uint level;

    uint grid_index;

    static clipmap_page unpack(uint packed)
    {
        clipmap_page page;

        uint global_id = packed & 0xFFFFF;
        uint local_id = global_id % SDF_CLIPMAP_PAGE_COUNT;

        uint N = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
        uint NN = N * N;

        uint z = local_id / NN;
        uint rem = local_id - z * NN;

        uint y = rem / N;
        uint x = rem - y * N;

        page.coord = uint3(x, y, z);
        page.level = global_id / SDF_CLIPMAP_PAGE_COUNT;

        page.grid_index = packed >> 20;

        return page;
    }

    uint pack()
    {
        uint global_id =
            coord.x +
            coord.y * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS +
            coord.z * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS +
            level * SDF_CLIPMAP_PAGE_COUNT;

        return grid_index << 20 | global_id;
    }

    uint3 get_page_table_coord()
    {
        return uint3(coord.xy, coord.z + level * SDF_CLIPMAP_PAGE_COUNT_PER_AXIS);
    }
};

static const uint SDF_CLIPMAP_PAGE_TABLE_ENTRY_FLAG_RESIDENT = 1 << 0;

struct clipmap_page_table_entry
{
    uint page_atlas_id;
    uint flags;

    static clipmap_page_table_entry unpack(uint packed)
    {
        clipmap_page_table_entry entry;
        entry.page_atlas_id = packed >> 11;
        entry.flags = packed & 0x7FF;

        return entry;
    }

    uint pack()
    {
        uint packed = 0;
        packed |= page_atlas_id << 11;
        packed |= flags;

        return packed;
    }

    uint3 get_page_atlas_coord()
    {
        uint3 coord;
        coord.x = page_atlas_id >> 14;
        coord.y = (page_atlas_id >> 7) & 0x7F;
        coord.z = page_atlas_id & 0x7F;
        return coord;
    }

    void set_page_atlas_coord(uint3 coord)
    {
        page_atlas_id = coord.x << 14 | coord.y << 7 | coord.z;
    }

    bool resident()
    {
        return valid() && (flags & SDF_CLIPMAP_PAGE_TABLE_ENTRY_FLAG_RESIDENT);
    }

    bool valid()
    {
        return flags != 0x7FF;
    }
};

uint get_clipmap_level(float3 position, float3 camera_position)
{
    float page_extent = SDF_CLIPMAP_PAGE_EXTENT;
    for (uint level = 0; level < SDF_CLIPMAP_LEVEL_COUNT; ++level)
    {
        int3 camera_page_coord = floor(camera_position / page_extent);
        int3 target_page_coord = floor(position / page_extent);

        int3 clipmap_min = camera_page_coord - SDF_CLIPMAP_PAGE_COUNT_PER_AXIS_HALF;
        int3 clipmap_max = camera_page_coord + SDF_CLIPMAP_PAGE_COUNT_PER_AXIS_HALF - 1;

        if (all(target_page_coord >= clipmap_min) && all(target_page_coord <= clipmap_max))
        {
            return level;
        }

        page_extent *= 2.0;
    }

    return 0xFFFFFFFF;
}

bool intersect_aabb(
    float3 aabb_min_a,
    float3 aabb_max_a,
    float3 aabb_min_b,
    float3 aabb_max_b,
    float influence_radius = 0.0)
{
    float3 gap = max(max(aabb_min_a - aabb_max_b, aabb_min_b - aabb_max_a), 0.0);
    return dot(gap, gap) <= influence_radius * influence_radius;
}

#endif