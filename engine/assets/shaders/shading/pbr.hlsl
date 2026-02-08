#include "common.hlsli"
#include "brdf.hlsli"
#include "shading/shading_model.hlsli"
#include "shadow.hlsli"

struct constant_data
{
    constant_common common;
    uint brdf_lut;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

float3 gtao_multi_bounce(float visibility, float3 albedo)
{
 	float3 a =  2.0404 * albedo - 0.3324;   
    float3 b = -4.7951 * albedo + 0.6417;
    float3 c =  2.7552 * albedo + 0.6903;
    
    float3 x = visibility.xxx;
    return max(x, ((x * a + b) * x + c) * x);
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    uint2 coord;
    if (!get_shading_coord(constant.common, gtid, gid, coord))
    {
        return;
    }

    float3 N;
    if (!unpack_gbuffer_normal(constant.common, coord, N))
    {
        return;
    }

    float3 albedo = unpack_gbuffer_albedo(constant.common, coord);

    float roughness;
    float metallic;
    unpack_gbuffer_material(constant.common, coord, roughness, metallic);

    float3 emissive = unpack_gbuffer_emissive(constant.common, coord);

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    
    float2 texcoord = get_compute_texcoord(coord, constant.common.width, constant.common.height);
    float3 position = reconstruct_position(constant.common.auxiliary_buffers[0], texcoord, camera.matrix_vp_inv).xyz;

    float3 F0 = lerp(0.04, albedo, metallic);

    float3 V = normalize(camera.position - position);

    float NdotV = saturate(dot(N, V));

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];

    shadow_context shadow = shadow_context::create(scene, camera);

    float3 direct_lighting = 0.0;
    for (int i = 0; i < scene.light_count; ++i)
    {
        light_data light = lights[i];

        float3 L = -light.direction;
        float3 H = normalize(L + V);

        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
        float VdotH = saturate(dot(V, H));

        float d = d_ggx(NdotH, roughness);
        float vis = v_smith_joint_approx(NdotV, NdotL, roughness);
        float3 f = f_schlick(VdotH, F0);
        float3 kd = lerp(1.0 - f, 0.0, metallic);

        float3 specular = d * vis * f;
        float3 diffuse = albedo / PI * kd;

        float shadow_factor = 1.0;
        if (light.vsm_address != 0xFFFFFFFF)
        {
            shadow_factor = shadow.get_shadow(light, position);
        }
        direct_lighting += (specular + diffuse) * NdotL * light.color * shadow_factor;
    }

    float3 ambient_lighting = 0.0;
    {
        float3 R = reflect(-V, N);
        float3 f = f_schlick_roughness(NdotV, F0, roughness);
        float3 kd = lerp(1.0 - f, 0.0, metallic);

        TextureCube<float3> prefilter_map = ResourceDescriptorHeap[scene.prefilter];
        float3 prefilter = prefilter_map.SampleLevel(linear_clamp_sampler, R, roughness * 4.0);

        Texture2D<float2> brdf_lut = ResourceDescriptorHeap[constant.brdf_lut];
        float2 brdf = brdf_lut.SampleLevel(linear_clamp_sampler, float2(NdotV, roughness), 0.0);
        float3 specular = F0 * brdf.x + brdf.y;

        TextureCube<float3> irradiance_map = ResourceDescriptorHeap[scene.irradiance];
        float3 irradiance = irradiance_map.SampleLevel(linear_clamp_sampler, N, 0.0);
        float3 diffuse = albedo * kd / PI;

        ambient_lighting = specular * prefilter + diffuse * irradiance;
    }

#ifdef USE_AO_BUFFER
    Texture2D<float> ao_buffer = ResourceDescriptorHeap[constant.common.auxiliary_buffers[1]];
    ambient_lighting *= gtao_multi_bounce(ao_buffer[coord], albedo);
#endif

    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.common.render_target];
    render_target[coord] = float4(direct_lighting + ambient_lighting + emissive, 1.0);
}