#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_dispatch_buffer;
    uint visible_light_count;
    uint lru_state;
    uint lru_curr_index;
};
PushConstant(constant_data, constant);

[numthreads(1, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<dispatch_command> vsm_dispatch_commands = ResourceDescriptorHeap[constant.vsm_dispatch_buffer];

    dispatch_command command;
    command.x = VIRTUAL_PAGE_TABLE_SIZE / 8;
    command.y = VIRTUAL_PAGE_TABLE_SIZE / 8;
    command.z = 0;
    vsm_dispatch_commands[0] = command;

    RWStructuredBuffer<uint> visible_light_count = ResourceDescriptorHeap[constant.visible_light_count];
    visible_light_count[0] = 0;
    visible_light_count[1] = 0;

    RWStructuredBuffer<vsm_lru_state> lru_states = ResourceDescriptorHeap[constant.lru_state];
    lru_states[constant.lru_curr_index].head = 0;
    lru_states[constant.lru_curr_index].tail = 0;
}