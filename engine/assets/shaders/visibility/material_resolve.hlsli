#ifndef MATERIAL_RESOLVE_HLSLI
#define MATERIAL_RESOLVE_HLSLI

#include "common.hlsli"
#include "visibility/visibility_utils.hlsli"
#include "gbuffer.hlsli"
#include "mesh.hlsli"

struct constant_data
{
    uint gbuffers[8];

    uint visibility_buffer;
    uint worklist_buffer;
    uint material_offset_buffer;
    uint material_index;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

template <typename T>
void resolve_material(uint3 gtid, uint3 gid)
{
    StructuredBuffer<uint> material_offsets = ResourceDescriptorHeap[constant.material_offset_buffer];
    StructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];

    uint material_index = constant.material_index;
    uint tile_index = worklist[material_offsets[material_index] + gid.x];
    
    Texture2D<uint2> visibility_buffer = ResourceDescriptorHeap[constant.visibility_buffer];

    uint width;
    uint height;
    visibility_buffer.GetDimensions(width, height);

    uint2 coord = uint2(tile_index >> 16, tile_index & 0xFFFF) * 8 + gtid.xy;
    if (coord.x >= width || coord.y >= height)
    {
        return;
    }

    uint instance_id;
    uint primitive_id;
    unpack_visibility(visibility_buffer[coord], instance_id, primitive_id);

    if (instance_id == 0xFFFFFFFF)
    {
        return;
    }

    mesh mesh = mesh::create(instance_id, scene);
    uint material_address = mesh.get_material_address();

    material_info material_info = load_material_info(scene.material_buffer, material_address);
    if (material_info.material_index != material_index)
    {
        return;
    }

    float2 ddx;
    float2 ddy;
    vertex vertex = mesh.fetch_vertex(primitive_id, float2(coord), float2(width, height), ddx, ddy, camera.matrix_vp);

    T material = load_material<T>(scene.material_buffer, material_address);
    gbuffer gbuffer = material.resolve(vertex, ddx, ddy);
    gbuffer.shading_model = material_info.shading_model;
    gbuffer.pack(constant.gbuffers, coord);
}

#endif