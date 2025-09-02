#include "common.hlsli"

struct constant_data
{
    uint shading_dispatch_buffer;
    uint shading_model_count;
};
PushConstant(constant_data, constant);

[numthreads(32, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= constant.shading_model_count)
    {
        return;
    }

    RWStructuredBuffer<dispatch_command> shading_dispatch_commands = ResourceDescriptorHeap[constant.shading_dispatch_buffer];
    shading_dispatch_commands[dtid.x].x = 0;
    shading_dispatch_commands[dtid.x].y = 1;
    shading_dispatch_commands[dtid.x].z = 1;
}