#include "common.hlsli"

static const uint MAX_GROUP_COUNT = 128;

struct draw_command
{
    uint index_count;
    uint instance_count;
    uint index_offset;
    uint vertex_offset;
    uint instance_offset;
};
ByteAddressBuffer groups : register(t0, space0);
RWByteAddressBuffer cull_result : register(u1, space0);
RWStructuredBuffer<draw_command> command_buffer : register(u2, space0);
RWBuffer<uint> count_buffer : register(u3, space0);

ConstantBuffer<scene_data> scene : register(b0, space1);
StructuredBuffer<mesh_data> meshes : register(t1, space1);
StructuredBuffer<instance_data> instances : register(t2, space1);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= scene.instance_count)
    {
        return;
    }

    uint instance_index = dtid.x;

    instance_data instance = instances[instance_index];
    uint mesh_index = instance.mesh_index;

    if (cull_result.Load(mesh_index * 4) == 1)
    {
        uint group_index = instance.group_index;

        uint command_index = 0;
        InterlockedAdd(count_buffer[group_index], 1, command_index);
        command_index += groups.Load<uint>(group_index * 4);

        count_buffer[group_index] = 1;

        draw_command command;
        command.index_count = instance.index_count;
        command.instance_count = 1;
        command.index_offset = instance.index_offset;
        command.vertex_offset = instance.vertex_offset;
        command.instance_offset = instance_index;

        command_buffer[command_index] = command;
    }
}