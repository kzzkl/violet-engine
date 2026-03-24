#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_info;
    uint visible_virtual_page_list;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_page_table;
    uint vsm_bounds_buffer;
    uint lru_state;
    uint lru_buffer;
    uint lru_curr_index;
    uint clear_physical_page_list;
    uint clear_physical_page_texels_indirect_args;
};
PushConstant(constant_data, constant);

groupshared uint gs_allocate_page_count;
groupshared uint gs_allocate_page_index[64];
groupshared uint gs_clear_physical_page_list_offset;

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    StructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
    if (dtid.x >= vsm_info[0].visible_virtual_page_count)
    {
        return;
    }

    StructuredBuffer<uint> visible_virtual_page_list = ResourceDescriptorHeap[constant.visible_virtual_page_list];

    uint virtual_page_index = visible_virtual_page_list[dtid.x];

    uint vsm_id;
    uint2 virtual_page_coord;
    get_virtual_page_coord(virtual_page_index, vsm_id, virtual_page_coord);

    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    if (group_index == 0)
    {
        gs_allocate_page_count = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID) == 0)
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
        }
        else
        {
            uint free_physical_page_index = lru_buffer[get_lru_offset(constant.lru_curr_index) + lru_index];

            virtual_page.physical_page_coord = get_physical_page_coord(free_physical_page_index);
            virtual_page.flags = VIRTUAL_PAGE_FLAG_REQUEST;
            virtual_page_table[virtual_page_index] = virtual_page.pack();

            vsm_physical_page physical_page;
            physical_page.virtual_page_coord = virtual_page_coord + vsms[vsm_id].page_coord;
            physical_page.vsm_id = vsm_id;
            physical_page.flags = PHYSICAL_PAGE_FLAG_RESIDENT | PHYSICAL_PAGE_FLAG_REQUEST | PHYSICAL_PAGE_FLAG_HZB_DIRTY;
            physical_page_table[free_physical_page_index] = physical_page.pack();

            uint allocate_page_index = 0;
            InterlockedAdd(gs_allocate_page_count, 1, allocate_page_index);
            gs_allocate_page_index[allocate_page_index] = free_physical_page_index;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        RWStructuredBuffer<dispatch_command> clear_physical_page_texels_indirect_args = ResourceDescriptorHeap[constant.clear_physical_page_texels_indirect_args];
        InterlockedAdd(clear_physical_page_texels_indirect_args[0].z, gs_allocate_page_count, gs_clear_physical_page_list_offset);
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index < gs_allocate_page_count)
    {
        RWStructuredBuffer<uint> clear_physical_page_list = ResourceDescriptorHeap[constant.clear_physical_page_list];
        clear_physical_page_list[gs_clear_physical_page_list_offset + group_index] = gs_allocate_page_index[group_index];
    }

    RWStructuredBuffer<vsm_bounds> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];

    InterlockedMin(vsm_bounds[vsm_id].required_bounds.x, virtual_page_coord.x);
    InterlockedMin(vsm_bounds[vsm_id].required_bounds.y, virtual_page_coord.y);
    InterlockedMax(vsm_bounds[vsm_id].required_bounds.z, virtual_page_coord.x);
    InterlockedMax(vsm_bounds[vsm_id].required_bounds.w, virtual_page_coord.y);

    if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID) == 0)
    {
        InterlockedMin(vsm_bounds[vsm_id].invalidated_bounds.x, virtual_page_coord.x);
        InterlockedMin(vsm_bounds[vsm_id].invalidated_bounds.y, virtual_page_coord.y);
        InterlockedMax(vsm_bounds[vsm_id].invalidated_bounds.z, virtual_page_coord.x);
        InterlockedMax(vsm_bounds[vsm_id].invalidated_bounds.w, virtual_page_coord.y);
    }
}