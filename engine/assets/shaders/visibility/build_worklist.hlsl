#include "common.hlsli"
#include "visibility/visibility_utils.hlsli"

static const uint TILE_SIZE = 8;
static const uint MAX_MATERIAL_COUNT = 1024;
static const uint MAX_MATERIAL_BATCH_COUNT = MAX_MATERIAL_COUNT / 32;

struct constant_data
{
    uint visibility_buffer;
    uint worklist_buffer;
    uint worklist_size_buffer;
    uint material_offset_buffer;
    uint sort_dispatch_buffer;
    uint width;
    uint height;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);

groupshared uint gs_material_flags[MAX_MATERIAL_BATCH_COUNT];
groupshared uint gs_material_list[TILE_SIZE * TILE_SIZE];
groupshared uint gs_material_count;
groupshared uint gs_worklist_offset;

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupID, uint group_index : SV_GroupIndex)
{
    if (group_index < MAX_MATERIAL_BATCH_COUNT)
    {
        gs_material_flags[group_index] = 0;
    }

    gs_material_list[group_index] = 0;

    if (group_index == 0)
    {
        gs_material_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    if (dtid.x >= constant.width || dtid.y >= constant.height)
    {
        return;
    }

    Texture2D<uint2> visibility_buffer = ResourceDescriptorHeap[constant.visibility_buffer];

    uint instance_id;
    uint primitive_id;
    unpack_visibility(visibility_buffer[dtid.xy], instance_id, primitive_id);

    if (instance_id != 0xFFFFFFFF)
    {
        StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
        material_info material_info = load_material_info(scene.material_buffer, instances[instance_id].material_address);
        if (material_info.material_index != 0)
        {
            uint flag_index = material_info.material_index / 32;
            uint flag_bit = material_info.material_index % 32;
            uint flag = 0;
            InterlockedOr(gs_material_flags[flag_index], 1u << flag_bit, flag);

            if ((flag & (1u << flag_bit)) == 0)
            {
                uint material_list_index = 0;
                InterlockedAdd(gs_material_count, 1, material_list_index);
                gs_material_list[material_list_index] = material_info.material_index;
            }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index >= gs_material_count)
    {
        return;
    }

    RWStructuredBuffer<uint2> worklist = ResourceDescriptorHeap[constant.worklist_buffer];
    RWStructuredBuffer<uint> worklist_sizes = ResourceDescriptorHeap[constant.worklist_size_buffer];
    RWStructuredBuffer<uint> material_offsets = ResourceDescriptorHeap[constant.material_offset_buffer];
    RWStructuredBuffer<dispatch_command> sort_dispatch_commands = ResourceDescriptorHeap[constant.sort_dispatch_buffer];

    if (group_index == 0)
    {
        InterlockedAdd(worklist_sizes[0], gs_material_count, gs_worklist_offset);

        uint start = gs_worklist_offset;
        uint end = gs_worklist_offset + gs_material_count;
        uint dispatch_count = (end + 63) / 64 - (start + 63) / 64;
        if (dispatch_count > 0)
        {
            InterlockedAdd(sort_dispatch_commands[0].x, dispatch_count);
        }
    }

    // TODO: remove?
    GroupMemoryBarrierWithGroupSync();
    
    uint material_index = gs_material_list[group_index];
    InterlockedAdd(material_offsets[material_index], 1);

    uint tile_index = (gid.x << 16) | gid.y;
    uint worklist_index = gs_worklist_offset + group_index;
    worklist[worklist_index] = uint2(material_index, tile_index);
}