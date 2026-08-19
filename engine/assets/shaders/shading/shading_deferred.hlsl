#include "shader_configs/shading_model_config.hlsli"
#include "gbuffer.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"
#include "atmosphere/atmosphere.hlsli"
#include "spherical_harmonics.hlsli"

struct constant_data
{
    uint gbuffers[8];

    uint ao_buffer;
    uint depth_buffer;
    uint render_target;

    uint shading_model;
    uint worklist_buffer;
    uint worklist_offset;

    uint shadow_light_index;
    uint shadow_mask;
    uint stage;

    uint prefilter_map;
    uint irradiance_sh;
    uint sun_index;
    float planet_radius;
    float atmosphere_radius;
    uint transmittance_lut;
    uint indirect_diffuse;

#ifdef SHADING_MODEL_HAS_CONSTANT
    shading_model_constant_data shading_model_constant;
#endif
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

static const uint LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED = 0;
static const uint LIGHTING_STAGE_DIRECT_LIGHTING_UNSHADOWED = 1;
static const uint LIGHTING_STAGE_INDIRECT_LIGHTING = 2;

float3 get_sun_transmittance(constant_data constant, float3 position, float3 sun_direction)
{
    position += float3(0.0, constant.planet_radius, 0.0);
    position.y = max(position.y, constant.planet_radius + 1.0);

    if (ray_sphere_intersection(position, -sun_direction, 0.0, constant.planet_radius) > 0.0)
    {
        return 0.0;
    }

    float r = length(position);
    float mu = -sun_direction.y;

    float2 uv;
    get_transmittance_lut_uv(constant.planet_radius, constant.atmosphere_radius, r, mu, uv);

    Texture2D<float3> transmittance_lut = ResourceDescriptorHeap[constant.transmittance_lut];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    float3 transmittance = transmittance_lut.SampleLevel(linear_clamp_sampler, uv, 0.0);
    return transmittance;
}

[shader("compute")]
[numthreads(SHADING_TILE_SIZE, SHADING_TILE_SIZE, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
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

    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_ALBEDO]];
    Texture2D<float2> gbuffer_material = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_MATERIAL]];
    Texture2D<uint> gbuffer_normal = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_NORMAL]];
    Texture2D<float3> gbuffer_emissive = ResourceDescriptorHeap[constant.gbuffers[GBUFFER_EMISSIVE]];

    gbuffer gbuffer = gbuffer::unpack(gbuffer_albbedo[coord], gbuffer_material[coord], gbuffer_normal[coord], gbuffer_emissive[coord]);

    if (gbuffer.shading_model != constant.shading_model)
    {
        return;
    }
    
    float2 uv = get_compute_uv(coord, width, height);

    shading_context context;
    context.ao_buffer = constant.ao_buffer;
    context.coord = coord;
    context.prefilter_map = constant.prefilter_map;

    surface surface;
    surface.position_ws = reconstruct_position(constant.depth_buffer, uv, camera.matrix_vp_inv).xyz;
    surface.albedo = gbuffer.albedo;
    surface.opacity = 1.0;
    surface.roughness = gbuffer.roughness;
    surface.metallic = gbuffer.metallic;
    surface.emissive = gbuffer.emissive;
    surface.normal_ws = gbuffer.normal;

#ifdef SHADING_MODEL_HAS_CONSTANT
    shading_model model = shading_model::create(context, constant.shading_model_constant, surface, camera);
#else
    shading_model model = shading_model::create(context, surface, camera);
#endif

    float3 lighting = 0.0;

    if (constant.stage == LIGHTING_STAGE_DIRECT_LIGHTING_SHADOWED)
    {
        StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];
        Texture2D<float> shadow_mask = ResourceDescriptorHeap[constant.shadow_mask];

        light_data light = lights[constant.shadow_light_index];
        if (constant.shadow_light_index == constant.sun_index)
        {
            light.color *= get_sun_transmittance(constant, surface.position_ws, light.direction);
        }

        lighting = model.evaluate_direct_lighting(context, light, shadow_mask[coord]);
    }
    else if (constant.stage == LIGHTING_STAGE_DIRECT_LIGHTING_UNSHADOWED)
    {
        StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];
        for (int i = 0; i < scene.light_count; ++i)
        {
            light_data light = lights[i];
            if (light.cast_shadow)
            {
                continue;
            }

            if (i == constant.sun_index)
            {
                light.color *= get_sun_transmittance(constant, surface.position_ws, light.direction);
            }

            lighting += model.evaluate_direct_lighting(context, light, 1.0);
        }
    }
    else if (constant.stage == LIGHTING_STAGE_INDIRECT_LIGHTING)
    {
        float3 irradiance = 0.0;
        if (constant.indirect_diffuse != 0)
        {
            Texture2D<float3> indirect_diffuse = ResourceDescriptorHeap[constant.indirect_diffuse];
            irradiance = indirect_diffuse.SampleLevel(get_linear_clamp_sampler(), uv, 0.0);
        }
        else
        {
            StructuredBuffer<sh9> irradiance_sh = ResourceDescriptorHeap[constant.irradiance_sh];
            sh9 sh = irradiance_sh[0];
            irradiance = sh.evaluate(gbuffer.normal);
        }

        lighting = model.evaluate_indirect_lighting(context, irradiance);
    }

    render_target[coord] += float4(lighting, 0.0);
}