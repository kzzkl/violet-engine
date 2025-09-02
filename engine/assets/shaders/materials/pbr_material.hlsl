#include "mesh.hlsli"
#include "visibility/visibility_utils.hlsli"
#include "visibility/material_resolve.hlsli"
#include "shading/shading_model.hlsli"

struct pbr_material
{
    float3 albedo;
    uint albedo_texture;
    float roughness;
    uint roughness_metallic_texture;
    float metallic;
    uint normal_texture;
    float3 emissive;
    uint emissive_texture;
};

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

    mesh mesh = mesh::create(instance_id);
    uint material_address = mesh.get_material_address();

    material_info material_info = load_material_info(scene.material_buffer, material_address);
    if (material_info.material_index != material_index)
    {
        return;
    }

    float2 ddx;
    float2 ddy;
    vertex vertex = mesh.fetch_vertex(primitive_id, float2(coord), float2(constant.width, constant.height), ddx, ddy);

    pbr_material material = load_material<pbr_material>(scene.material_buffer, material_address);
    
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

    Texture2D<float3> albedo_texture = ResourceDescriptorHeap[material.albedo_texture];
    Texture2D<float3> roughness_metallic_texture = ResourceDescriptorHeap[material.roughness_metallic_texture];
    float3 roughness_metallic = roughness_metallic_texture.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy);

    Texture2D<float3> emissive_texture = ResourceDescriptorHeap[material.emissive_texture];
    float3 emissive = emissive_texture.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy);

    float3 N = vertex.normal_ws;
    if (material.normal_texture != 0)
    {
        Texture2D<float3> normal_texture = ResourceDescriptorHeap[material.normal_texture];

        float3 tangent_normal = normalize(normal_texture.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy) * 2.0 - 1.0);
        float3 n = normalize(vertex.normal_ws);
        float3 t = normalize(vertex.tangent_ws);
        float3 b = normalize(cross(n, t));
        float3x3 tbn = transpose(float3x3(t, b, n));
        N = normalize(mul(tbn, tangent_normal));
    }

    RWTexture2D<float4> gbuffer_albedo = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_ALBEDO]];
    RWTexture2D<float2> gbuffer_material = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_MATERIAL]];
    RWTexture2D<float4> gbuffer_emissive = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_EMISSIVE]];
    RWTexture2D<uint> gbuffer_normal = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_NORMAL]];

    gbuffer_albedo[coord] = float4(material.albedo * albedo_texture.SampleGrad(linear_repeat_sampler, vertex.texcoord, ddx, ddy), 1.0);    
    gbuffer_material[coord] = float2(material.roughness * roughness_metallic.g, material.metallic * roughness_metallic.b);
    gbuffer_emissive[coord] = float4(material.emissive * emissive, 1.0);
    gbuffer_normal[coord] = pack_gbuffer_normal(N, material_info.shading_model);
}