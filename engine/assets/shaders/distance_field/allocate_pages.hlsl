#include "common.hlsli"
#include "distance_field/sdf_common.hlsli"

struct constant_data
{
    uint clipmap_state;

    uint page_table;
    uint free_pages;

    uint pages_to_allocate;
    uint page_atlas_capacity;
};
PushConstant(constant_data, constant);

groupshared int gs_allocate_offset;
groupshared int gs_allocate_count;

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex, uint3 gid : SV_GroupID)
{
    RWStructuredBuffer<clipmap_state> clipmap_state = ResourceDescriptorHeap[constant.clipmap_state];
    StructuredBuffer<uint> pages_to_allocate = ResourceDescriptorHeap[constant.pages_to_allocate];

    RWTexture3D<uint> page_table = ResourceDescriptorHeap[constant.page_table];

    if (group_index == 0)
    {
        int allocate_count = 64;
        if ((gid.x + 1) * 64 > clipmap_state[0].pages_to_allocate_count)
        {
            allocate_count = clipmap_state[0].pages_to_allocate_count % 64;
        }

        int allocate_offset;
        InterlockedAdd(clipmap_state[0].free_page_atlas_count, -allocate_count, allocate_offset);

        if (allocate_offset - allocate_count < 0)
        {
            allocate_count = allocate_offset;
        }

        gs_allocate_offset = allocate_offset - allocate_count;
        gs_allocate_count = allocate_count;
    }

    GroupMemoryBarrierWithGroupSync();

    if (dtid.x < clipmap_state[0].pages_to_allocate_count && gs_allocate_count > 0)
    {
        uint page_table_entry;
        if (group_index < gs_allocate_count)
        {
            StructuredBuffer<uint> free_pages = ResourceDescriptorHeap[constant.free_pages];

            clipmap_page_table_entry entry;
            entry.page_atlas_id = free_pages[gs_allocate_offset + group_index];
            entry.flags = SDF_CLIPMAP_PAGE_TABLE_ENTRY_FLAG_RESIDENT;
            page_table_entry = entry.pack();
        }
        else
        {
            page_table_entry = SDF_INVALID_PAGE_TABLE_ENTRY;
        }

        clipmap_page page = clipmap_page::unpack(pages_to_allocate[dtid.x]);
        page_table[page.get_page_table_coord()] = page_table_entry;
    }
}