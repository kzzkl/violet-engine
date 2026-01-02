#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint physical_page_table;
    uint lru_state;
    uint lru_buffer;
    uint lru_curr_index;
    uint lru_prev_index;
    uint lru_remap;
};
PushConstant(constant_data, constant);

static const uint VSM_LRU_INVALID_MASK = 1 << 31;

[numthreads(64, 1, 1)]
void mark_invalid_pages(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<vsm_physical_page> physical_page_table = ResourceDescriptorHeap[constant.physical_page_table];
    
    StructuredBuffer<vsm_lru_state> lru_states = ResourceDescriptorHeap[constant.lru_state];
    RWStructuredBuffer<uint> lru_buffer = ResourceDescriptorHeap[constant.lru_buffer];

    RWStructuredBuffer<uint> lru_remap = ResourceDescriptorHeap[constant.lru_remap];

    vsm_lru_state prev_state = lru_states[constant.lru_prev_index];

    uint lru_index = dtid.x;
    if (lru_index >= prev_state.tail)
    {
        return;
    }

    uint lru_prev_offset = get_lru_offset(constant.lru_prev_index);

    vsm_physical_page physical_page = physical_page_table[lru_buffer[lru_prev_offset + lru_index]];

    if (lru_index < prev_state.head || (physical_page.flags & PHYSICAL_PAGE_FLAG_REQUEST))
    {
        physical_page.flags &= ~PHYSICAL_PAGE_FLAG_IN_LRU;
        physical_page_table[lru_buffer[lru_prev_offset + lru_index]] = physical_page;

        lru_buffer[lru_prev_offset + lru_index] |= VSM_LRU_INVALID_MASK;
        lru_remap[lru_index] = 1;
    }
    else
    {
        lru_remap[lru_index] = 0;
    }
}

[numthreads(64, 1, 1)]
void remove_invalid_pages(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<vsm_lru_state> lru_states = ResourceDescriptorHeap[constant.lru_state];

    uint prev_tail = lru_states[constant.lru_prev_index].tail;
    if (dtid.x >= prev_tail)
    {
        return;
    }

    RWStructuredBuffer<uint> lru_buffer = ResourceDescriptorHeap[constant.lru_buffer];
    StructuredBuffer<uint> lru_remap = ResourceDescriptorHeap[constant.lru_remap];

    uint lru_curr_offset = get_lru_offset(constant.lru_curr_index);
    uint lru_prev_offset = get_lru_offset(constant.lru_prev_index);

    uint index = dtid.x;

    if ((lru_buffer[lru_prev_offset + index] & VSM_LRU_INVALID_MASK) == 0)
    {
        lru_buffer[lru_curr_offset + index - lru_remap[index]] = lru_buffer[lru_prev_offset + index];
    }

    if (dtid.x == 0)
    {
        uint remove_count = lru_remap[prev_tail - 1];
        remove_count += lru_buffer[lru_prev_offset + prev_tail - 1] & VSM_LRU_INVALID_MASK ? 1 : 0;
        lru_states[constant.lru_curr_index].tail = prev_tail - remove_count;
    }
}

[numthreads(64, 1, 1)]
void append_unused_pages(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<vsm_physical_page> physical_page_table = ResourceDescriptorHeap[constant.physical_page_table];
    RWStructuredBuffer<vsm_lru_state> lru_states = ResourceDescriptorHeap[constant.lru_state];
    RWStructuredBuffer<uint> lru_buffer = ResourceDescriptorHeap[constant.lru_buffer];

    uint physical_page_index = dtid.x;
    if (physical_page_index >= PHYSICAL_PAGE_TABLE_ITEM_COUNT)
    {
        return;
    }

    vsm_physical_page physical_page = physical_page_table[physical_page_index];

    if ((physical_page.flags & (PHYSICAL_PAGE_FLAG_REQUEST | PHYSICAL_PAGE_FLAG_IN_LRU)) == 0)
    {
        physical_page.flags |= PHYSICAL_PAGE_FLAG_IN_LRU;
        physical_page_table[physical_page_index] = physical_page;

        uint lru_index = 0;
        InterlockedAdd(lru_states[constant.lru_curr_index].tail, 1, lru_index);
        lru_buffer[get_lru_offset(constant.lru_curr_index) + lru_index] = physical_page_index;
    }
}