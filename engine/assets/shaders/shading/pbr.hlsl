#include "common.hlsli"
#include "brdf.hlsli"
#include "shading/shading_model.hlsli"

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

struct pbr_shading_model
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 emissive;
    float3 position;
    float3 F0;

    float3 N;
    float3 V;
    float NdotV;

    uint2 coord;

    static pbr_shading_model create(gbuffer gbuffer, uint2 coord)
    {
        pbr_shading_model shading_model;
        shading_model.albedo = gbuffer.albedo;
        shading_model.roughness = gbuffer.roughness;
        shading_model.metallic = gbuffer.metallic;
        shading_model.emissive = gbuffer.emissive;
        shading_model.position = gbuffer.position;

        shading_model.F0 = lerp(0.04, gbuffer.albedo, gbuffer.metallic);

        shading_model.N = gbuffer.normal;
        shading_model.V = normalize(camera.position - gbuffer.position);
        shading_model.NdotV = saturate(dot(shading_model.N, shading_model.V));

        shading_model.coord = coord;

        return shading_model;
    }

    float3 evaluate_direct_lighting(light_data light, float shadow)
    {
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

        return (specular + diffuse) * NdotL * light.color * shadow;
    }

    float3 evaluate_indirect_lighting()
    {
        float3 R = reflect(-V, N);
        float3 f = f_schlick_roughness(NdotV, F0, roughness);
        float3 kd = lerp(1.0 - f, 0.0, metallic);

        SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

        TextureCube<float3> prefilter_map = ResourceDescriptorHeap[scene.prefilter];
        float3 prefilter = prefilter_map.SampleLevel(linear_clamp_sampler, R, roughness * 4.0);

        Texture2D<float2> brdf_lut = ResourceDescriptorHeap[constant.brdf_lut];
        float2 brdf = brdf_lut.SampleLevel(linear_clamp_sampler, float2(NdotV, roughness), 0.0);
        float3 specular = F0 * brdf.x + brdf.y;

        TextureCube<float3> irradiance_map = ResourceDescriptorHeap[scene.irradiance];
        float3 irradiance = irradiance_map.SampleLevel(linear_clamp_sampler, N, 0.0);
        float3 diffuse = albedo * kd / PI;

        float3 lighting = specular * prefilter + diffuse * irradiance;

#ifdef USE_AO_BUFFER
        Texture2D<float> ao_buffer = ResourceDescriptorHeap[constant.common.auxiliary_buffers[1]];
        lighting *= gtao_multi_bounce(ao_buffer[coord], albedo);
#endif

        return lighting + emissive;
    }
};

[numthreads(SHADING_TILE_SIZE, SHADING_TILE_SIZE, 1)]
void cs_main(uint3 gtid : SV_GroupThreadID, uint3 gid : SV_GroupID)
{
    evaluate_lighting<pbr_shading_model>(constant.common, scene, camera, gtid, gid);
}