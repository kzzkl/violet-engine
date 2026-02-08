#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_vsm_ids;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_page_table;
    uint lru_state;
    uint lru_buffer;
    uint lru_curr_index;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];

    uint2 virtual_page_coord = dtid.xy;
    uint vsm_id = vsm_ids[dtid.z];

    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    uint virtual_page_index = get_virtual_page_index(vsm_id, virtual_page_coord);

    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) != 0 && (virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID) == 0)
    {
        StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];

        RWStructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];
        RWStructuredBuffer<vsm_lru_state> lru_states = ResourceDescriptorHeap[constant.lru_state];
        StructuredBuffer<uint> lru_buffer = ResourceDescriptorHeap[constant.lru_buffer];

        uint lru_index = 0;
        InterlockedAdd(lru_states[constant.lru_curr_index].head, 1, lru_index);

        if (lru_index >= lru_states[constant.lru_curr_index].tail)
        {
            virtual_page.flags = VIRTUAL_PAGE_FLAG_REQUEST | VIRTUAL_PAGE_FLAG_UNMAPPED;
            virtual_page_table[virtual_page_index] = virtual_page.pack();
            return;
        }

        uint free_physical_page_index = lru_buffer[get_lru_offset(constant.lru_curr_index) + lru_index];

        virtual_page.physical_page_coord.x = free_physical_page_index % PHYSICAL_PAGE_TABLE_SIZE;
        virtual_page.physical_page_coord.y = free_physical_page_index / PHYSICAL_PAGE_TABLE_SIZE;
        virtual_page.flags = VIRTUAL_PAGE_FLAG_REQUEST;
        virtual_page_table[virtual_page_index] = virtual_page.pack();

        vsm_physical_page physical_page;
        physical_page.virtual_page_coord = virtual_page_coord + vsms[vsm_id].page_coord;
        physical_page.vsm_id = vsm_id;
        physical_page.flags = PHYSICAL_PAGE_FLAG_RESIDENT | PHYSICAL_PAGE_FLAG_REQUEST | PHYSICAL_PAGE_FLAG_HZB_DIRTY;
        physical_page.frame = 0;
        physical_page_table[free_physical_page_index] = physical_page.pack();
    }
}