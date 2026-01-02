#ifndef VSM_COMMON_HLSLI
#define VSM_COMMON_HLSLI

static const uint VIRTUAL_PAGE_TABLE_SIZE = 32;
static const uint VIRTUAL_PAGE_TABLE_ITEM_COUNT = VIRTUAL_PAGE_TABLE_SIZE * VIRTUAL_PAGE_TABLE_SIZE;
static const uint VIRTUAL_PAGE_SIZE = 128;

static const uint PHYSICAL_PAGE_TABLE_SIZE = 32;
static const uint PHYSICAL_PAGE_TABLE_ITEM_COUNT = PHYSICAL_PAGE_TABLE_SIZE * PHYSICAL_PAGE_TABLE_SIZE;

static const uint DIRECTIONAL_VSM_CASCADE_FIRST = 7;
static const uint DIRECTIONAL_VSM_CASCADE_LAST = 22;

static const uint MAX_CAMERA_COUNT = 16;
static const uint MAX_SHADOW_LIGHT_COUNT = 32;
static const uint MAX_VSM_COUNT = 256;

struct vsm_data
{
    int2 page_coord;
    int2 page_offset;
    float4x4 matrix_vp;
};

static const uint VIRTUAL_PAGE_FLAG_REQUEST = 1 << 0;
static const uint VIRTUAL_PAGE_FLAG_CACHE_VALID = 1 << 1;
static const uint VIRTUAL_PAGE_FLAG_UNMAPPED = 1 << 2;

struct vsm_virtual_page
{
    uint2 physical_page_coord;
    uint flags;
};

static const uint PHYSICAL_PAGE_FLAG_REQUEST = 1 << 0;
static const uint PHYSICAL_PAGE_FLAG_RESIDENT = 1 << 1;
static const uint PHYSICAL_PAGE_FLAG_IN_LRU = 1 << 2;

struct vsm_physical_page
{
    int2 virtual_page_coord;
    uint vsm_id;
    uint flags;
};

struct vsm_draw_info
{
    uint vsm_id;
    uint instance_id;
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
    return vsm_id * VIRTUAL_PAGE_TABLE_ITEM_COUNT + page_coord.y * VIRTUAL_PAGE_TABLE_SIZE + page_coord.x;
}

uint get_physical_page_index(uint2 page_coord)
{
    return page_coord.y * PHYSICAL_PAGE_TABLE_SIZE + page_coord.x;
}

uint get_directional_cascade(float distance)
{
    return clamp(ceil(log2(distance)), DIRECTIONAL_VSM_CASCADE_FIRST, DIRECTIONAL_VSM_CASCADE_LAST) - DIRECTIONAL_VSM_CASCADE_FIRST;
}

uint pack_virtual_page(vsm_virtual_page virtual_page)
{
    uint packed_data = 0;
    packed_data |= (virtual_page.physical_page_coord.x << 24);
    packed_data |= (virtual_page.physical_page_coord.y << 16);
    packed_data |= virtual_page.flags;
    return packed_data;
}

vsm_virtual_page unpack_virtual_page(uint packed_data)
{
    vsm_virtual_page virtual_page;
    virtual_page.physical_page_coord.x = (packed_data & 0xFF000000) >> 24;
    virtual_page.physical_page_coord.y = (packed_data & 0x00FF0000) >> 16;
    virtual_page.flags = packed_data & 0x0000FFFF;
    return virtual_page;
}

uint get_lru_offset(uint lru_index)
{
    return lru_index * PHYSICAL_PAGE_TABLE_ITEM_COUNT;
}

#endif