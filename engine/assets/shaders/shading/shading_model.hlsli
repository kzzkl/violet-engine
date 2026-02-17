#ifndef SHADING_MODEL_HLSLI
#define SHADING_MODEL_HLSLI

#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_common
{
    uint gbuffers[8];
    uint auxiliary_buffers[4];
    uint render_target;
    uint width;
    uint height;
    uint shading_model;
    uint worklist_buffer;
    uint worklist_offset;
    uint light_id;
    uint shadow_mask;
    uint stage;
    uint padding0;
    uint padding1;
    uint padding2;
};

static const uint LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED = 0;
static const uint LIGHTING_STAGE_DIRECT_LIGHTING_UNSHADOWED = 1;
static const uint LIGHTING_STAGE_INDIRECT_LIGHTING = 2;

bool get_shading_coord(constant_common constant, uint3 gtid, uint3 gid, out uint2 coord)
{
    StructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];

    uint tile_index = worklist[constant.worklist_offset + gid.x];
    coord = uint2(tile_index >> 16, tile_index & 0xFFFF) * SHADING_TILE_SIZE + gtid.xy;
    return coord.x < constant.width && coord.y < constant.height;
}

float3 unpack_gbuffer_albedo(constant_common constant, uint2 coord)
{
    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_ALBEDO]];
    return gbuffer_albbedo[coord].rgb;
}

void unpack_gbuffer_material(constant_common constant, uint2 coord, out float roughness, out float metallic)
{
    Texture2D<float2> gbuffer_material = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_MATERIAL]];
    roughness = max(gbuffer_material[coord].x, 0.03);
    metallic = gbuffer_material[coord].y;
}

uint pack_gbuffer_normal(float3 N, uint shading_model)
{
    float2 oct = normal_to_octahedron(N);
    return (uint(oct.x * 4095.0) << 20) | (uint(oct.y * 4095.0) << 8) | shading_model;
}

bool unpack_gbuffer_normal(constant_common constant, uint2 coord, out float3 N)
{
    Texture2D<uint> gbuffer_normal = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_NORMAL]];
    uint pack = gbuffer_normal[coord];

    uint shading_model = pack & 0xFF;
    if (shading_model != constant.shading_model)
    {
        return false;
    }

    float2 oct = float2(float(pack >> 20) / 4095.0, float((pack & 0x000FFF00) >> 8) / 4095.0);
    N = octahedron_to_normal(oct);

    return true;
}

float3 unpack_gbuffer_normal(uint gbuffer_normal, uint2 coord)
{
    Texture2D<uint> buffer = ResourceDescriptorHeap[gbuffer_normal];
    uint pack = buffer[coord];

    float2 oct = float2(float(pack >> 20) / 4095.0, float((pack & 0x000FFF00) >> 8) / 4095.0);
    return octahedron_to_normal(oct);
}

float3 unpack_gbuffer_emissive(constant_common constant, uint2 coord)
{
    Texture2D<float4> gbuffer_emissive = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_EMISSIVE]];
    return gbuffer_emissive[coord].rgb;
}

struct gbuffer
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 emissive;
    float3 normal;
    float3 position;
};

template <typename ShadingModel>
void evaluate_lighting(constant_common constant, scene_data scene, camera_data camera, uint3 gtid, uint3 gid)
{
    StructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];

    uint tile_index = worklist[constant.worklist_offset + gid.x];
    uint2 coord = uint2(tile_index >> 16, tile_index & 0xFFFF) * SHADING_TILE_SIZE + gtid.xy;
    if (coord.x >= constant.width || coord.y >= constant.height)
    {
        return;
    }

    gbuffer gbuffer;
    
    if (!unpack_gbuffer_normal(constant, coord, gbuffer.normal))
    {
        return;
    }

    gbuffer.albedo = unpack_gbuffer_albedo(constant, coord);
    gbuffer.emissive = unpack_gbuffer_emissive(constant, coord);
    unpack_gbuffer_material(constant, coord, gbuffer.roughness, gbuffer.metallic);

    float2 texcoord = get_compute_texcoord(coord, constant.width, constant.height);
    gbuffer.position = reconstruct_position(constant.auxiliary_buffers[0], texcoord, camera.matrix_vp_inv).xyz;

    ShadingModel shading_model = ShadingModel::create(gbuffer, coord);
    float3 lighting = 0.0;

    if (constant.stage == LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED)
    {
        StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
        Texture2D<float> shadow_mask = ResourceDescriptorHeap[constant.shadow_mask];

        light_data light = lights[constant.light_id];
        lighting = shading_model.evaluate_direct_lighting(light, shadow_mask[coord]);
    }
    else if (constant.stage == LIGHTING_STAGE_DIRECT_LIGHTING_UNSHADOWED)
    {
        StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.non_shadow_casting_light_buffer];
        for (int i = 0; i < scene.non_shadow_casting_light_count; ++i)
        {
            lighting += shading_model.evaluate_direct_lighting(lights[i], 1.0);
        }
    }
    else if (constant.stage == LIGHTING_STAGE_INDIRECT_LIGHTING)
    {
        lighting = shading_model.evaluate_indirect_lighting();
    }

    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];
    render_target[coord] += float4(lighting, 1.0);
}

#endif