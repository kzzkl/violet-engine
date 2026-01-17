#include "mesh.hlsli"
#include "visibility/visibility_utils.hlsli"
#include "visibility/material_resolve.hlsli"
#include "shading/shading_model.hlsli"

struct unlit_material
{
    float3 albedo;
};

ConstantBuffer<scene_data> scene : register(b0, space1);

[numthreads(8, 8, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    StructuredBuffer<uint> material_offsets = ResourceDescriptorHeap[constant.material_offset_buffer];
    StructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];

    uint material_index = constant.material_index;
    uint tile_index = worklist[material_offsets[material_index] + gid.x];

    uint2 coord = uint2(tile_index >> 16, tile_index & 0xFFFF) * 8 + gtid.xy;
    if (coord.x >= constant.width || coord.y >= constant.height)
    {
        return;
    }

    Texture2D<uint2> visibility_buffer = ResourceDescriptorHeap[constant.visibility_buffer];
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

    unlit_material material = load_material<unlit_material>(scene.material_buffer, material_address);

    RWTexture2D<float4> gbuffer_albedo = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_ALBEDO]];
    RWTexture2D<uint> gbuffer_normal = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_NORMAL]];

    gbuffer_albedo[coord] = float4(material.albedo, 1.0);
    gbuffer_normal[coord] = pack_gbuffer_normal(float3(0.0, 0.0, 0.0), material_info.shading_model);
}