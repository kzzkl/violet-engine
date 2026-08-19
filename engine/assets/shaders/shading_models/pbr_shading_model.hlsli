#include "brdf.hlsli"
#include "shading/shading_model.hlsli"
#include "spherical_harmonics.hlsli"

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
    struct constant_data
    {
        uint brdf_lut;
    };

    float3 albedo;
    float roughness;
    float metallic;
    float3 emissive;
    float3 position;
    float3 F0;

    float3 N;
    float3 V;
    float NdotV;

    uint brdf_lut_texture;

    static pbr_shading_model create(shading_context context, constant_data constant, surface surface, camera_data camera)
    {
        pbr_shading_model shading_model;
        shading_model.albedo = surface.albedo;
        shading_model.roughness = surface.roughness;
        shading_model.metallic = surface.metallic;
        shading_model.emissive = surface.emissive;
        shading_model.position = surface.position_ws;

        shading_model.F0 = lerp(0.04, surface.albedo, surface.metallic);

        shading_model.N = surface.normal_ws;
        shading_model.V = normalize(camera.position - surface.position_ws);
        shading_model.NdotV = saturate(dot(shading_model.N, shading_model.V));

        shading_model.brdf_lut_texture = constant.brdf_lut;

        return shading_model;
    }

    float3 evaluate_direct_lighting(shading_context context, light_data light, float shadow)
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

    float3 evaluate_indirect_lighting(shading_context context, float3 irradiance)
    {
        float3 R = reflect(-V, N);
        float3 f = f_schlick_roughness(NdotV, F0, roughness);
        float3 kd = lerp(1.0 - f, 0.0, metallic);

        SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

        TextureCube<float3> prefilter_map = ResourceDescriptorHeap[context.prefilter_map];

        uint prefilter_width;
        uint prefilter_height;
        uint level_count;
        prefilter_map.GetDimensions(0, prefilter_width, prefilter_height, level_count);

        float3 prefilter = prefilter_map.SampleLevel(linear_clamp_sampler, R, roughness * (level_count - 1));

        Texture2D<float2> brdf_lut = ResourceDescriptorHeap[brdf_lut_texture];
        float2 brdf = brdf_lut.SampleLevel(linear_clamp_sampler, float2(NdotV, roughness), 0.0);

        float3 specular = (F0 * brdf.x + brdf.y) * prefilter;
        float3 diffuse = albedo * kd * irradiance;

        if (context.ao_buffer != 0)
        {
            Texture2D<float> ao_buffer = ResourceDescriptorHeap[context.ao_buffer];
            float diffuse_ao = ao_buffer[context.coord];
            diffuse *= gtao_multi_bounce(diffuse_ao, albedo);

            float specular_ao = saturate(pow(NdotV + diffuse_ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + diffuse_ao);
            specular *= specular_ao;
        }

        return specular + diffuse + emissive;
    }
};