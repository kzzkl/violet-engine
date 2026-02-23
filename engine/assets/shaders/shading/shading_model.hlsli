#ifndef SHADING_MODEL_HLSLI
#define SHADING_MODEL_HLSLI

#include "common.hlsli"
#include "gbuffer.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_common
{
    uint gbuffers[8];
    uint auxiliary_buffers[4];
    uint render_target;
    uint shading_model;
    uint worklist_buffer;
    uint worklist_offset;
    uint light_id;
    uint shadow_mask;
    uint stage;
    uint padding0;
};

static const uint LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED = 0;
static const uint LIGHTING_STAGE_DIRECT_LIGHTING_UNSHADOWED = 1;
static const uint LIGHTING_STAGE_INDIRECT_LIGHTING = 2;

template <typename ShadingModel>
void evaluate_lighting(constant_common constant, scene_data scene, camera_data camera, uint3 gtid, uint3 gid)
{
    StructuredBuffer<uint> worklist = ResourceDescriptorHeap[constant.worklist_buffer];

    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];
    uint width;
    uint height;
    render_target.GetDimensions(width, height);

    uint tile_index = worklist[constant.worklist_offset + gid.x];
    uint2 coord = uint2(tile_index >> 16, tile_index & 0xFFFF) * SHADING_TILE_SIZE + gtid.xy;
    if (coord.x >= width || coord.y >= height)
    {
        return;
    }

    gbuffer gbuffer = gbuffer::unpack(constant.gbuffers, coord);
    if (gbuffer.shading_model != constant.shading_model)
    {
        return;
    }

    float2 texcoord = get_compute_texcoord(coord, width, height);
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

    render_target[coord] += float4(lighting, 0.0);
}

#endif