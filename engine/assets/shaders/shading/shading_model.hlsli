#ifndef SHADING_MODEL_HLSLI
#define SHADING_MODEL_HLSLI

#include "material.hlsli"

struct shading_context
{
    uint ao_buffer;

    uint2 coord;
    uint prefilter_map;
};

#endif