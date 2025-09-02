#ifndef VISIBILITY_UTILS_HLSLI
#define VISIBILITY_UTILS_HLSLI

uint2 pack_visibility(uint instance_id, uint primitive_id)
{
    return uint2(instance_id, primitive_id);
}

void unpack_visibility(uint2 packed, out uint instance_id, out uint primitive_id)
{
    instance_id = packed.x;
    primitive_id = packed.y;
}

#endif