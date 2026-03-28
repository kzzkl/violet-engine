#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"
#include "cluster.hlsli"

struct constant_data
{
    uint virtual_pages_indirect_args;
    uint visible_virtual_pages_indirect_args;
    uint visible_virtual_page_texels_indirect_args;
    uint clear_physical_page_texels_indirect_args;

    uint vsm_info;
    uint lru_state;
    uint lru_curr_index;
    uint draw_count_buffer;
    uint cluster_queue_state;
};
PushConstant(constant_data, constant);

[numthreads(1, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    dispatch_command command;

    RWStructuredBuffer<dispatch_command> virtual_pages_indirect_args = ResourceDescriptorHeap[constant.virtual_pages_indirect_args];
    command.x = VIRTUAL_PAGE_TABLE_SIZE / 8;
    command.y = VIRTUAL_PAGE_TABLE_SIZE / 8;
    command.z = 0;
    virtual_pages_indirect_args[0] = command;

    RWStructuredBuffer<dispatch_command> visible_virtual_pages_indirect_args = ResourceDescriptorHeap[constant.visible_virtual_pages_indirect_args];
    command.x = 0;
    command.y = 1;
    command.z = 1;
    visible_virtual_pages_indirect_args[0] = command;

    RWStructuredBuffer<dispatch_command> visible_virtual_page_texels_indirect_args = ResourceDescriptorHeap[constant.visible_virtual_page_texels_indirect_args];
    command.x = PAGE_RESOLUTION / 16;
    command.y = PAGE_RESOLUTION / 16;
    command.z = 0;
    visible_virtual_page_texels_indirect_args[0] = command;

    RWStructuredBuffer<dispatch_command> clear_physical_page_texels_indirect_args = ResourceDescriptorHeap[constant.clear_physical_page_texels_indirect_args];
    command.x = PAGE_RESOLUTION / 16;
    command.y = PAGE_RESOLUTION / 16;
    command.z = 0;
    clear_physical_page_texels_indirect_args[0] = command;

    RWStructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
    vsm_info[0].visible_light_count = 0;
    vsm_info[0].visible_vsm_count = 0;
    vsm_info[0].visible_virtual_page_count = 0;

    RWStructuredBuffer<vsm_lru_state> lru_states = ResourceDescriptorHeap[constant.lru_state];
    lru_states[constant.lru_curr_index].head = 0;
    lru_states[constant.lru_curr_index].tail = 0;

    RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];
    draw_counts[0] = 0;
    draw_counts[1] = 0;
    draw_counts[2] = 0;
    draw_counts[3] = 0;

    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_states = ResourceDescriptorHeap[constant.cluster_queue_state];
    cluster_queue_states[0] = (cluster_queue_state_data)0;
}