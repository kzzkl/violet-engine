#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"
#include "cluster.hlsli"

struct constant_data
{
    uint virtual_page_dispatch_buffer;
    uint visible_light_count;
    uint lru_state;
    uint lru_curr_index;
    uint draw_count_buffer;
    uint clear_physical_page_dispatch_buffer;
    uint cluster_queue_state;
};
PushConstant(constant_data, constant);

[numthreads(1, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<dispatch_command> dispatch_commands = ResourceDescriptorHeap[constant.virtual_page_dispatch_buffer];

    dispatch_command command;
    command.x = VIRTUAL_PAGE_TABLE_SIZE / 8;
    command.y = VIRTUAL_PAGE_TABLE_SIZE / 8;
    command.z = 0;
    dispatch_commands[0] = command;

    RWStructuredBuffer<uint> visible_light_count = ResourceDescriptorHeap[constant.visible_light_count];
    visible_light_count[0] = 0;
    visible_light_count[1] = 0;

    RWStructuredBuffer<vsm_lru_state> lru_states = ResourceDescriptorHeap[constant.lru_state];
    lru_states[constant.lru_curr_index].head = 0;
    lru_states[constant.lru_curr_index].tail = 0;

    RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];
    draw_counts[0] = 0;

    RWStructuredBuffer<dispatch_command> clear_physical_page_commands = ResourceDescriptorHeap[constant.clear_physical_page_dispatch_buffer];
    clear_physical_page_commands[0].x = PAGE_RESOLUTION / 8;
    clear_physical_page_commands[0].y = PAGE_RESOLUTION / 8;
    clear_physical_page_commands[0].z = 0;

    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_states = ResourceDescriptorHeap[constant.cluster_queue_state];
    cluster_queue_states[0] = (cluster_queue_state_data)0;
}