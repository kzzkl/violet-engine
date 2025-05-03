#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);

struct constant_data
{
    uint cull_result;
    uint command_buffer;
    uint count_buffer;
    uint padding0;
};
PushConstant(constant_data, constant);

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

    RWByteAddressBuffer cull_result = ResourceDescriptorHeap[constant.cull_result];
    if (cull_result.Load(instance.mesh_index * 4) == 1)
    {
        StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
        mesh_data mesh = meshes[instance.mesh_index];

        ByteAddressBuffer batch_buffer = ResourceDescriptorHeap[scene.batch_buffer];
        RWBuffer<uint> count_buffer = ResourceDescriptorHeap[constant.count_buffer];

        uint batch_index = instance.batch_index;

        uint command_index = 0;
        InterlockedAdd(count_buffer[batch_index], 1, command_index);
        command_index += batch_buffer.Load<uint>(batch_index * 4);

        draw_command command;
        command.index_count = instance.index_count;
        command.instance_count = 1;
        command.index_offset = instance.index_offset + mesh.index_offset;
        command.vertex_offset = instance.vertex_offset;
        command.instance_offset = instance_index;

        RWStructuredBuffer<draw_command> command_buffer = ResourceDescriptorHeap[constant.command_buffer];
        command_buffer[command_index] = command;
    }
}