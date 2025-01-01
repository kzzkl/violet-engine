#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);

struct fill_data
{
    uint cull_result;
    uint command_buffer;
    uint count_buffer;
    uint padding0;
};
ConstantBuffer<fill_data> fill : register(b0, space2);

struct draw_command
{
    uint index_count;
    uint instance_count;
    uint index_offset;
    uint vertex_offset;
    uint instance_offset;
};

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= scene.instance_count)
    {
        return;
    }

    uint instance_index = dtid.x;

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    uint mesh_index = instance.mesh_index;

    RWByteAddressBuffer cull_result = ResourceDescriptorHeap[fill.cull_result];

    if (cull_result.Load(mesh_index * 4) == 1)
    {
        ByteAddressBuffer group_buffer = ResourceDescriptorHeap[scene.group_buffer];
        RWBuffer<uint> count_buffer = ResourceDescriptorHeap[fill.count_buffer];

        uint group_index = instance.group_index;

        uint command_index = 0;
        InterlockedAdd(count_buffer[group_index], 1, command_index);
        command_index += group_buffer.Load<uint>(group_index * 4);

        draw_command command;
        command.index_count = instance.index_count;
        command.instance_count = 1;
        command.index_offset = instance.index_offset;
        command.vertex_offset = instance.vertex_offset;
        command.instance_offset = instance_index;

        RWStructuredBuffer<draw_command> command_buffer = ResourceDescriptorHeap[fill.command_buffer];
        command_buffer[command_index] = command;
    }
}