#include "common.hlsli"

struct constant_data
{
    uint cluster_queue;
    uint cluster_queue_size;
};
PushConstant(constant_data, constant);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint index = dtid.x;

    if (index < constant.cluster_queue_size)
    {
        RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
        cluster_queue[index] = uint2(0xFFFFFFFF, 0xFFFFFFFF);
    }
}