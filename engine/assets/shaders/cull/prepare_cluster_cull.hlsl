#include "common.hlsli"
#include "cull/cull.hlsli"

struct constant_data
{
    uint cluster_queue_state;
    uint dispatch_command_buffer;
};
PushConstant(constant_data, constant);

[numthreads(1, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];
    RWStructuredBuffer<dispatch_command> dispatch_commands = ResourceDescriptorHeap[constant.dispatch_command_buffer];

    dispatch_command command;

#if defined(CULL_CLUSTER_NODE)
    uint cluster_node_queue_rear = cluster_queue_state[0].cluster_node_queue_rear;
    uint cluster_node_queue_prev_rear = cluster_queue_state[0].cluster_node_queue_prev_rear;

    cluster_queue_state[0].cluster_node_queue_front = cluster_node_queue_prev_rear;
    cluster_queue_state[0].cluster_node_queue_prev_rear = cluster_node_queue_rear;

    uint cluster_node_count = cluster_node_queue_rear - cluster_node_queue_prev_rear;
    command.x = (cluster_node_count + MAX_NODE_PER_GROUP - 1) / MAX_NODE_PER_GROUP;

#elif defined(CULL_CLUSTER)
    uint cluster_queue_rear = cluster_queue_state[0].cluster_queue_rear;
    command.x = (cluster_queue_rear + CLUSTER_CULL_GROUP_SIZE - 1) / CLUSTER_CULL_GROUP_SIZE;
#endif

    command.y = 1;
    command.z = 1;
    dispatch_commands[0] = command;
}