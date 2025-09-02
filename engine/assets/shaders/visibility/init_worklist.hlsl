#include "common.hlsli"

struct constant_data
{
    uint worklist_size_buffer;
    uint sort_dispatch_buffer;
    uint resolve_dispatch_buffer;
    uint material_offset_buffer;
    uint material_count;
};
PushConstant(constant_data, constant);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= constant.material_count)
    {
        return;
    }

    if (dtid.x == 0)
    {
        RWStructuredBuffer<dispatch_command> sort_dispatch_commands = ResourceDescriptorHeap[constant.sort_dispatch_buffer];
        sort_dispatch_commands[0].x = 0;
        sort_dispatch_commands[0].y = 1;
        sort_dispatch_commands[0].z = 1;

        RWStructuredBuffer<uint> worklist_sizes = ResourceDescriptorHeap[constant.worklist_size_buffer];
        worklist_sizes[0] = 0;
    }

    RWStructuredBuffer<dispatch_command> resolve_dispatch_commands = ResourceDescriptorHeap[constant.resolve_dispatch_buffer];
    resolve_dispatch_commands[dtid.x].x = 0;
    resolve_dispatch_commands[dtid.x].y = 1;
    resolve_dispatch_commands[dtid.x].z = 1;

    RWStructuredBuffer<uint> material_offsets = ResourceDescriptorHeap[constant.material_offset_buffer];
    material_offsets[dtid.x] = 0;
}