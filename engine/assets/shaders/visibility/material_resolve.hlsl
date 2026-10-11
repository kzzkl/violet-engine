#ifndef MATERIAL_RESOLVE_HLSLI
#define MATERIAL_RESOLVE_HLSLI

#define USE_RASTER_INTERPOLATION 0

#include "shader_configs/material_config.hlsli"
#include "material.hlsli"
#include "visibility/visibility_utils.hlsli"
#include "gbuffer.hlsli"

struct constant_data
{
    uint gbuffers[8];

    uint visibility_buffer;
    uint worklist_buffer;
    uint material_offset_buffer;
    uint resolve_pipeline;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    StructuredBuffer<uint> material_offsets = ResourceDescriptorHeap[constant.material_offset_buffer];
    StructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];

    uint resolve_pipeline = constant.resolve_pipeline;
    uint tile_index = worklist[material_offsets[resolve_pipeline] + gid.x];
    
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
    
    material_context context;
    context.camera = camera;
    context.scene = scene;

    mesh mesh = mesh::create(instance_id, scene, primitive_id, float2(coord), float2(width, height), camera.matrix_vp, context.ddx, context.ddy);

    material_data material = load_material<material_data>(scene.material_buffer, mesh.get_material_address());
    if (material.common.get_resolve_pipeline() != resolve_pipeline)
    {
        return;
    }

    material::varying varying = material.data.evaluate_varying(context, mesh);
    surface surface = material.data.evaluate_surface(context, varying);

    gbuffer gbuffer;
    gbuffer.albedo = surface.albedo;
    gbuffer.roughness = surface.roughness;
    gbuffer.metallic = surface.metallic;
    gbuffer.emissive = surface.emissive;
    gbuffer.normal = surface.normal_ws;
    gbuffer.shading_model = material.common.get_shading_model();

    RWTexture2D<float4> gbuffer_albedo = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_ALBEDO]];
    RWTexture2D<float2> gbuffer_material = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_MATERIAL]];
    RWTexture2D<float3> gbuffer_emissive = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_EMISSIVE]];
    RWTexture2D<uint> gbuffer_normal = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_NORMAL]];

    gbuffer.pack(gbuffer_albedo[coord], gbuffer_material[coord], gbuffer_normal[coord], gbuffer_emissive[coord]);
}

#endif