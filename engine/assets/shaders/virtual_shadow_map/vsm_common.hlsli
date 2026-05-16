#ifndef VSM_COMMON_HLSLI
#define VSM_COMMON_HLSLI

static const uint VIRTUAL_PAGE_TABLE_SIZE = 64;
static const uint VIRTUAL_PAGE_TABLE_PAGE_COUNT = VIRTUAL_PAGE_TABLE_SIZE * VIRTUAL_PAGE_TABLE_SIZE;
static const float VIRTUAL_PAGE_TABLE_SIZE_INV = 1.0 / VIRTUAL_PAGE_TABLE_SIZE;

static const uint PHYSICAL_PAGE_TABLE_SIZE_X = 64;
static const uint PHYSICAL_PAGE_TABLE_SIZE_Y = 32;
static const uint PHYSICAL_PAGE_TABLE_PAGE_COUNT = PHYSICAL_PAGE_TABLE_SIZE_X * PHYSICAL_PAGE_TABLE_SIZE_Y;

static const uint DIRECTIONAL_VSM_CASCADE_FIRST = 7;
static const uint DIRECTIONAL_VSM_CASCADE_LAST = 22;

static const uint PAGE_WORLD_SIZE = 1.28 * 2.0 / VIRTUAL_PAGE_TABLE_SIZE;
static const uint PAGE_RESOLUTION = 128;

static const uint VIRTUAL_RESOLUTION = VIRTUAL_PAGE_TABLE_SIZE * PAGE_RESOLUTION;
static const uint2 PHYSICAL_RESOLUTION = uint2(PHYSICAL_PAGE_TABLE_SIZE_X, PHYSICAL_PAGE_TABLE_SIZE_Y) * PAGE_RESOLUTION;

static const float VIRTUAL_TEXEL_SIZE = 1.0 / VIRTUAL_RESOLUTION;

static const uint MAX_CAMERA_COUNT = 16;
static const uint MAX_SHADOW_LIGHT_COUNT = 32;
static const uint MAX_VSM_COUNT = 256;

static const uint MAX_SHADOW_DRAWS_PER_FRAME = 1024 * 100;

static const uint MAX_SHADOW_DRAWS_PER_BATCH = 1024 * 20;
static const uint SHADOW_BATCH_COUNT = 6;

static const uint STATIC_DRAW_COMMAND_OFFSET = 0;
static const uint STATIC_DRAW_COUNT_OFFSET = 0;

static const uint DYNAMIC_DRAW_COMMAND_OFFSET = MAX_SHADOW_DRAWS_PER_BATCH * SHADOW_BATCH_COUNT;
static const uint DYNAMIC_DRAW_COUNT_OFFSET = SHADOW_BATCH_COUNT;

struct vsm_data
{
    int2 page_coord;
    uint cache_epoch;
    float view_z_radius;
    float4x4 matrix_v;
    float4x4 matrix_p;
    float4x4 matrix_vp;
    float texel_size;
    float texel_size_inv;
    uint cascade_index;
    uint cascade_count;
};

static const uint VIRTUAL_PAGE_FLAG_VISIBLE = 1 << 0;
static const uint VIRTUAL_PAGE_FLAG_RESIDENT = 1 << 1;
static const uint VIRTUAL_PAGE_FLAG_RENDERING = 1 << 2;
static const uint VIRTUAL_PAGE_FLAG_UNMAPPED = 1 << 3;
static const uint VIRTUAL_PAGE_FLAG_VALID = VIRTUAL_PAGE_FLAG_RESIDENT | VIRTUAL_PAGE_FLAG_RENDERING;

struct vsm_info
{
    uint visible_light_count;
    uint visible_vsm_count;
    uint visible_virtual_page_count;
    uint render_virtual_page_count;
};

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

    bool resident()
    {
        return (flags & VIRTUAL_PAGE_FLAG_RESIDENT) != 0;
    }

    bool valid()
    {
        return flags & VIRTUAL_PAGE_FLAG_VALID;
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
    uint flags;

    static vsm_physical_page unpack(uint4 packed_data)
    {
        vsm_physical_page physical_page;
        physical_page.virtual_page_coord = packed_data.xy;
        physical_page.vsm_id = packed_data.z;
        physical_page.flags = packed_data.w;
        return physical_page;
    }

    uint4 pack()
    {
        uint4 packed_data = 0;
        packed_data.xy = virtual_page_coord;
        packed_data.z = vsm_id;
        packed_data.w = flags;
        return packed_data;
    }
};

struct vsm_draw_info
{
    uint vsm_id;
    uint instance_id;
    uint cluster_id;
    uint padding;
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

void get_virtual_page_coord(uint virtual_page_index, out uint vsm_id, out uint2 page_coord)
{
    vsm_id = virtual_page_index / VIRTUAL_PAGE_TABLE_PAGE_COUNT;

    uint local_page_index = virtual_page_index - vsm_id * VIRTUAL_PAGE_TABLE_PAGE_COUNT;
    page_coord = uint2(local_page_index % VIRTUAL_PAGE_TABLE_SIZE, local_page_index / VIRTUAL_PAGE_TABLE_SIZE);
}

uint get_physical_page_index(uint2 page_coord)
{
    return page_coord.y * PHYSICAL_PAGE_TABLE_SIZE_X + page_coord.x;
}

uint2 get_physical_page_coord(uint physical_page_index)
{
    return uint2(physical_page_index % PHYSICAL_PAGE_TABLE_SIZE_X, physical_page_index / PHYSICAL_PAGE_TABLE_SIZE_X);
}

uint get_directional_cascade(float distance)
{
    return clamp(ceil(log2(distance * 100)), DIRECTIONAL_VSM_CASCADE_FIRST, DIRECTIONAL_VSM_CASCADE_LAST) - DIRECTIONAL_VSM_CASCADE_FIRST;
}

uint get_lru_offset(uint lru_index)
{
    return lru_index * PHYSICAL_PAGE_TABLE_PAGE_COUNT;
}

struct vsm_sample_result
{
    bool valid;
    float depth;
};

vsm_sample_result vsm_sample_depth(
    uint vsm_id,
    float2 uv,
    Texture2D<uint> physical_shadow_map,
    StructuredBuffer<uint> virtual_page_table)
{
    float2 virtual_page_coord_f = uv * VIRTUAL_PAGE_TABLE_SIZE;
    uint2 virtual_page_coord = floor(virtual_page_coord_f);
    float2 virtual_page_local_uv = frac(virtual_page_coord_f);

    uint virtual_page_index = get_virtual_page_index(vsm_id, virtual_page_coord);

    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    uint2 physical_texel = virtual_page.get_physical_texel(virtual_page_local_uv);

    vsm_sample_result result;
    result.valid = virtual_page.valid();
    result.depth = asfloat(physical_shadow_map[physical_texel]);

    return result;
}

#endif