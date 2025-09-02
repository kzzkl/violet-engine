#include "common.hlsli"

struct constant_data
{
    uint raw_worklist_buffer;
    uint worklist_buffer;
    uint worklist_size_buffer;
    uint material_offset_buffer;
    uint resolve_dispatch_buffer;
};
PushConstant(constant_data, constant);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> worklist_sizes = ResourceDescriptorHeap[constant.worklist_size_buffer];
    if (dtid.x >= worklist_sizes[0])
    {
        return;
    }

    StructuredBuffer<uint2> raw_worklist = ResourceDescriptorHeap[constant.raw_worklist_buffer];
    uint material_index = raw_worklist[dtid.x].x;
    uint tile_index = raw_worklist[dtid.x].y;

    RWStructuredBuffer<dispatch_command> resolve_dispatch_commands = ResourceDescriptorHeap[constant.resolve_dispatch_buffer];
    uint worklist_offset = 0;
    InterlockedAdd(resolve_dispatch_commands[material_index].x, 1, worklist_offset);

    StructuredBuffer<uint> material_offsets = ResourceDescriptorHeap[constant.material_offset_buffer];
    uint worklist_index =  worklist_offset + material_offsets[material_index];

    RWStructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];
    worklist[worklist_index] = tile_index;
}