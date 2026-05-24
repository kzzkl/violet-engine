#include "common.hlsli"

struct constant_data
{
    uint recheck_count;
    uint dispatch_buffer;
};
PushConstant(constant_data, constant);

[shader("compute")]
[numthreads(1, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> recheck_count = ResourceDescriptorHeap[constant.recheck_count];
    RWStructuredBuffer<dispatch_command> dispatch_commands = ResourceDescriptorHeap[constant.dispatch_buffer];

    dispatch_command command;
    command.x = (recheck_count[0] + 63) / 64;
    command.y = 1;
    command.z = 1;
    dispatch_commands[0] = command;
}