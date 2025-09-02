#ifndef MATERIAL_RESOLVE_HLSLI
#define MATERIAL_RESOLVE_HLSLI

#include "common.hlsli"

struct constant_data
{
    uint gbuffers[8];

    uint visibility_buffer;
    uint worklist_buffer;
    uint material_offset_buffer;

    uint material_index;
    uint width;
    uint height;
};
PushConstant(constant_data, constant);

#endif