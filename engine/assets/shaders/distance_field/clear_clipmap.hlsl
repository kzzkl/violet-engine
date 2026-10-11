#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint clipmap_state;
    uint page_table;
    uint free_pages;
    uint page_atlas_capacity;
};
PushConstant(constant_data, constant);

[shader("compute")]
[numthreads(4, 4, 4)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture3D<uint> page_table = ResourceDescriptorHeap[constant.page_table];
    page_table[dtid] = SDF_INVALID_PAGE_TABLE_ENTRY;

    if (all(dtid == 0))
    {
        RWStructuredBuffer<clipmap_state> clipmap_state = ResourceDescriptorHeap[constant.clipmap_state];
        clipmap_state[0].free_page_atlas_count = constant.page_atlas_capacity;
    }

    uint dispatch_size = SDF_CLIPMAP_PAGE_COUNT_PER_AXIS;
    uint index = dtid.x + dtid.y * dispatch_size + dtid.z * dispatch_size * dispatch_size;
    if (index < constant.page_atlas_capacity)
    {
        RWStructuredBuffer<uint> free_pages = ResourceDescriptorHeap[constant.free_pages];
        free_pages[index] = index;
    }
}