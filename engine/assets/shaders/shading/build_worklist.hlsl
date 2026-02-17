#include "common.hlsli"

static const uint MAX_SHADING_MODEL_COUNT = 256;
static const uint MAX_SHADING_MODEL_BATCH_COUNT = MAX_SHADING_MODEL_COUNT / 32;

struct constant_data
{
    uint gbuffer_normal;
    uint worklist_buffer;
    uint shading_dispatch_buffer;
    uint tile_count;
    uint width;
    uint height;
};
PushConstant(constant_data, constant);

groupshared uint gs_shading_model_flags[MAX_SHADING_MODEL_BATCH_COUNT];
groupshared uint gs_shading_model_list[SHADING_TILE_SIZE * SHADING_TILE_SIZE];
groupshared uint gs_shading_model_count;

[numthreads(SHADING_TILE_SIZE, SHADING_TILE_SIZE, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint3 gid : SV_GroupID, uint group_index : SV_GroupIndex)
{
    if (group_index < MAX_SHADING_MODEL_BATCH_COUNT)
    {
        gs_shading_model_flags[group_index] = 0;
    }

    gs_shading_model_list[group_index] = 0;

    if (group_index == 0)
    {
        gs_shading_model_count = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    if (dtid.x >= constant.width || dtid.y >= constant.height)
    {
        return;
    }

    Texture2D<uint> shading_model_buffer = ResourceDescriptorHeap[constant.gbuffer_normal];
    uint shading_model = shading_model_buffer[dtid.xy] & 0xFF;

    if (shading_model != 0xFF)
    {
        uint flag_index = shading_model / 32;
        uint flag_bit = shading_model % 32;
        uint flag = 0;
        InterlockedOr(gs_shading_model_flags[flag_index], 1u << flag_bit, flag);

        if ((flag & (1u << flag_bit)) == 0)
        {
            uint shading_model_list_index = 0;
            InterlockedAdd(gs_shading_model_count, 1, shading_model_list_index);
            gs_shading_model_list[shading_model_list_index] = shading_model;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index >= gs_shading_model_count)
    {
        return;
    }

    RWStructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];
    RWStructuredBuffer<dispatch_command> shading_dispatch_commands = ResourceDescriptorHeap[constant.shading_dispatch_buffer];

    shading_model = gs_shading_model_list[group_index];

    uint worklist_index = 0;
    InterlockedAdd(shading_dispatch_commands[shading_model].x, 1, worklist_index);
    worklist_index += shading_model * constant.tile_count;
    worklist[worklist_index] = (gid.x << 16) | gid.y;
}