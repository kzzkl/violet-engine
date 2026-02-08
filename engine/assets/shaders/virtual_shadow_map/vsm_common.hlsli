#ifndef VSM_COMMON_HLSLI
#define VSM_COMMON_HLSLI

static const uint VIRTUAL_PAGE_TABLE_SIZE = 64;
static const uint VIRTUAL_PAGE_TABLE_PAGE_COUNT = VIRTUAL_PAGE_TABLE_SIZE * VIRTUAL_PAGE_TABLE_SIZE;
static const float VIRTUAL_PAGE_TABLE_SIZE_INV = 1.0 / VIRTUAL_PAGE_TABLE_SIZE;

static const uint PHYSICAL_PAGE_TABLE_SIZE = 32;
static const uint PHYSICAL_PAGE_TABLE_PAGE_COUNT = PHYSICAL_PAGE_TABLE_SIZE * PHYSICAL_PAGE_TABLE_SIZE;

static const uint DIRECTIONAL_VSM_CASCADE_FIRST = 7;
static const uint DIRECTIONAL_VSM_CASCADE_LAST = 22;

static const uint PAGE_WORLD_SIZE = 1.28 * 2.0 / VIRTUAL_PAGE_TABLE_SIZE;
static const uint PAGE_RESOLUTION = 128;

static const uint VIRTUAL_RESOLUTION = VIRTUAL_PAGE_TABLE_SIZE * PAGE_RESOLUTION;
static const uint PHYSICAL_RESOLUTION = PHYSICAL_PAGE_TABLE_SIZE * PAGE_RESOLUTION;

static const uint MAX_CAMERA_COUNT = 16;
static const uint MAX_SHADOW_LIGHT_COUNT = 32;
static const uint MAX_VSM_COUNT = 256;

static const uint MAX_SHADOW_DRAWS_PER_FRAME = 1024 * 100;
static const uint STATIC_INSTANCE_DRAW_OFFSET = 0;
static const uint DYNAMIC_INSTANCE_DRAW_OFFSET = MAX_SHADOW_DRAWS_PER_FRAME;

struct vsm_data
{
    int2 page_coord;
    uint cache_epoch;
    float view_z_radius;
    float4x4 matrix_v;
    float4x4 matrix_p;
    float4x4 matrix_vp;
    float pixels_per_unit;
    uint padding0;
    uint padding1;
    uint padding2;
};

static const uint VIRTUAL_PAGE_FLAG_REQUEST = 1 << 0;
static const uint VIRTUAL_PAGE_FLAG_CACHE_VALID = 1 << 1;
static const uint VIRTUAL_PAGE_FLAG_UNMAPPED = 1 << 2;

struct vsm_bounds
{
    uint4 required_bounds;
    uint4 invalidated_bounds;
};

struct vsm_virtual_page
{
    uint2 physical_page_coord;
    uint flags;

    static vsm_virtual_page unpack(uint packed_data)
    {
        vsm_virtual_page virtual_page;
        virtual_page.physical_page_coord.x = (packed_data & 0xFF000000) >> 24;
        virtual_page.physical_page_coord.y = (packed_data & 0x00FF0000) >> 16;
        virtual_page.flags = packed_data & 0x0000FFFF;
        return virtual_page;
    }

    uint pack()
    {
        uint packed_data = 0;
        packed_data |= (physical_page_coord.x << 24);
        packed_data |= (physical_page_coord.y << 16);
        packed_data |= flags;
        return packed_data;
    }

    uint2 get_physical_texel(float2 virtual_page_local_uv)
    {
        return physical_page_coord * PAGE_RESOLUTION + uint2(virtual_page_local_uv * PAGE_RESOLUTION);
    }
};

static const uint PHYSICAL_PAGE_FLAG_REQUEST = 1 << 0;
static const uint PHYSICAL_PAGE_FLAG_RESIDENT = 1 << 1;
static const uint PHYSICAL_PAGE_FLAG_IN_LRU = 1 << 2;
static const uint PHYSICAL_PAGE_FLAG_HZB_DIRTY = 1 << 3;

struct vsm_physical_page
{
    int2 virtual_page_coord;
    uint vsm_id;
    uint frame;
    uint flags;

    static vsm_physical_page unpack(uint4 packed_data)
    {
        vsm_physical_page physical_page;
        physical_page.virtual_page_coord = packed_data.xy;
        physical_page.vsm_id = packed_data.z & 0xFFFF;
        physical_page.frame = packed_data.z >> 16;
        physical_page.flags = packed_data.w;
        return physical_page;
    }

    uint4 pack()
    {
        uint4 packed_data = 0;
        packed_data.xy = virtual_page_coord;
        packed_data.z = vsm_id | (frame << 16);
        packed_data.w = flags;
        return packed_data;
    }
};

struct vsm_draw_info
{
    uint vsm_id;
    uint instance_id;
    uint cluster_id;
};

struct vsm_lru_state
{
    uint head;
    uint tail;
};

uint get_directional_vsm_id(StructuredBuffer<uint> directional_vsms, uint vsm_address, uint camera_id)
{
    return directional_vsms[vsm_address * MAX_CAMERA_COUNT + camera_id];
}

uint get_virtual_page_index(uint vsm_id, uint2 page_coord)
{
    return vsm_id * VIRTUAL_PAGE_TABLE_PAGE_COUNT + page_coord.y * VIRTUAL_PAGE_TABLE_SIZE + page_coord.x;
}

uint get_physical_page_index(uint2 page_coord)
{
    return page_coord.y * PHYSICAL_PAGE_TABLE_SIZE + page_coord.x;
}

uint get_directional_cascade(float distance)
{
    return clamp(ceil(log2(distance)), DIRECTIONAL_VSM_CASCADE_FIRST, DIRECTIONAL_VSM_CASCADE_LAST) - DIRECTIONAL_VSM_CASCADE_FIRST;
}

uint get_lru_offset(uint lru_index)
{
    return lru_index * PHYSICAL_PAGE_TABLE_PAGE_COUNT;
}

uint2 get_physical_page_coord(uint physical_page_index)
{
    return uint2(physical_page_index % PHYSICAL_PAGE_TABLE_SIZE, physical_page_index / PHYSICAL_PAGE_TABLE_SIZE);
}

#endif