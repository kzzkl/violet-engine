#include "common.hlsli"

static const uint GROUP_SIZE = 128;

struct constant_data
{
    uint buffer;
    uint size;
    uint offset_buffer;
};
PushConstant(constant_data, constant);

groupshared uint gs_data[GROUP_SIZE];

[numthreads(GROUP_SIZE, 1, 1)]
void scan(uint gi : SV_GroupIndex, uint3 gid : SV_GroupID)
{
    RWStructuredBuffer<uint> buffer = ResourceDescriptorHeap[constant.buffer];

    uint buffer_offset = gid.x * GROUP_SIZE;
    if (buffer_offset + gi >= constant.size)
    {
        return;
    }

    uint value = buffer_offset + gi > constant.size ? 0 : buffer[buffer_offset + gi];
    gs_data[gi] = value;

    GroupMemoryBarrierWithGroupSync();

    uint group_offset = 0;
    for (uint i = 1; i < GROUP_SIZE; i <<= 1)
    {
        if (gi & i)
        {
            gs_data[gi] += gs_data[((gi >> group_offset) << group_offset) - 1];
        }
        ++group_offset;

        GroupMemoryBarrierWithGroupSync();
    }

    buffer[buffer_offset + gi] = gs_data[gi] - value;

#if defined(OUTPUT_OFFSET)
    if (gi == 0)
    {
        RWStructuredBuffer<uint> offset_buffer = ResourceDescriptorHeap[constant.offset_buffer];
        offset_buffer[gid.x] = gs_data[GROUP_SIZE - 1];
    }
#endif
}

groupshared uint gs_offset;

[numthreads(GROUP_SIZE, 1, 1)]
void scan_offset(uint gi : SV_GroupIndex, uint3 gid : SV_GroupID)
{
    RWStructuredBuffer<uint> buffer = ResourceDescriptorHeap[constant.buffer];
    RWStructuredBuffer<uint> offset_buffer = ResourceDescriptorHeap[constant.offset_buffer];

    if (gi == 0)
    {
        gs_offset = 0;
        for (uint i = 0; i <= gid.x; ++i)
        {
            gs_offset += offset_buffer[i];
        }
    }

    GroupMemoryBarrierWithGroupSync();

    uint buffer_offset = (gid.x + 1) * GROUP_SIZE;
    buffer[buffer_offset + gi] += gs_offset;
}