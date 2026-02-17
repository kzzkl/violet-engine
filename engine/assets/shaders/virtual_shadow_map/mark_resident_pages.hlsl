#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_virtual_page_table;
    uint vsm_physical_page_table;
    uint vsm_buffer;
    uint frame;
};
PushConstant(constant_data, constant);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];

    uint physical_page_index = dtid.x;

    vsm_physical_page physical_page = vsm_physical_page::unpack(physical_page_table[physical_page_index]);

    // Skip empty page.
    if ((physical_page.flags & PHYSICAL_PAGE_FLAG_RESIDENT) == 0)
    {
        return;
    }

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[physical_page.vsm_id];

    if (vsm.cache_epoch == constant.frame)
    {
        physical_page.flags &= ~(PHYSICAL_PAGE_FLAG_REQUEST | PHYSICAL_PAGE_FLAG_RESIDENT);
        physical_page.flags |= PHYSICAL_PAGE_FLAG_NEED_CLEAR;
        physical_page_table[physical_page_index] = physical_page.pack();
        return;
    }

    int2 virtual_page_coord = physical_page.virtual_page_coord - vsm.page_coord;

    if (virtual_page_coord.x >= VIRTUAL_PAGE_TABLE_SIZE || virtual_page_coord.y >= VIRTUAL_PAGE_TABLE_SIZE ||
        virtual_page_coord.x < 0 || virtual_page_coord.y < 0)
    {
        physical_page.flags &= ~PHYSICAL_PAGE_FLAG_REQUEST;
    }
    else
    {
        RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
        uint page_index = get_virtual_page_index(physical_page.vsm_id, virtual_page_coord);

        vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[page_index]);
        virtual_page.physical_page_coord = get_physical_page_coord(physical_page_index);

        if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) == 0)
        {
            physical_page.flags &= ~PHYSICAL_PAGE_FLAG_REQUEST;
        }
        else
        {
            physical_page.flags |= PHYSICAL_PAGE_FLAG_REQUEST;
            physical_page.flags &= ~PHYSICAL_PAGE_FLAG_NEED_CLEAR;
            virtual_page.flags |= VIRTUAL_PAGE_FLAG_CACHE_VALID;
        }

        virtual_page_table[page_index] = virtual_page.pack();
    }

    physical_page_table[physical_page_index] = physical_page.pack();
}