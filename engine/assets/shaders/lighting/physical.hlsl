#include "common.hlsli"
#include "brdf.hlsli"

static const float MIN_ROUGHNESS = 0.03;

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);
struct constant_data
{
    uint albedo;
    uint material;
    uint normal;
    uint depth;
    uint emissive;
    uint ao_buffer;
    uint brdf_lut;
};
ConstantBuffer<constant_data> constant : register(b0, space3);

float3 gtao_multi_bounce(float visibility, float3 albedo)
{
 	float3 a =  2.0404 * albedo - 0.3324;   
    float3 b = -4.7951 * albedo + 0.6417;
    float3 c =  2.7552 * albedo + 0.6903;
    
    float3 x = visibility.xxx;
    return max(x, ((x * a + b) * x + c) * x);
}

float4 fs_main(float2 texcoord : TEXCOORD) : SV_TARGET
{
    SamplerState point_clamp_sampler = get_point_clamp_sampler();
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[constant.albedo];
    float3 albedo = gbuffer_albbedo.Sample(point_clamp_sampler, texcoord).rgb;

    Texture2D<float2> gbuffer_material = ResourceDescriptorHeap[constant.material];
    float2 material = gbuffer_material.Sample(point_clamp_sampler, texcoord);
    float roughness = max(material.x, MIN_ROUGHNESS);
    float metallic = material.y;

    Texture2D<float2> gbuffer_normal = ResourceDescriptorHeap[constant.normal];
    float3 N = octahedron_to_normal(gbuffer_normal.Sample(point_clamp_sampler, texcoord));

    Texture2D<float4> gbuffer_emissive = ResourceDescriptorHeap[constant.emissive];
    float3 emissive = gbuffer_emissive.Sample(point_clamp_sampler, texcoord).rgb;
    
    float3 position = reconstruct_position(constant.depth, texcoord, camera.view_projection_inv).xyz;

    float3 F0 = lerp(0.04, albedo, metallic);

    float3 V = normalize(camera.position - position);

    float NdotV = saturate(dot(N, V));

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];

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

        direct_lighting += (specular + diffuse) * NdotL * light.color;
    }

    float3 ambient_lighting = 0.0;
    {
        float3 R = reflect(-V, N);
        float3 f = f_schlick_roughness(NdotV, F0, roughness);
        float3 kd = lerp(1.0 - f, 0.0, metallic);

        TextureCube<float3> prefilter_map = ResourceDescriptorHeap[scene.prefilter];
        float3 prefilter = prefilter_map.SampleLevel(linear_clamp_sampler, R, roughness * 4.0);

        Texture2D<float2> brdf_lut = ResourceDescriptorHeap[constant.brdf_lut];
        float2 brdf = brdf_lut.Sample(linear_clamp_sampler, float2(NdotV, roughness));
        float3 specular = (F0 * brdf.x + brdf.y) * prefilter;

        TextureCube<float3> irradiance_map = ResourceDescriptorHeap[scene.irradiance];
        float3 irradiance = irradiance_map.Sample(linear_clamp_sampler, N);
        float3 diffuse = albedo * irradiance * kd / PI;

        ambient_lighting = specular + diffuse;
    }

#ifdef USE_AO_BUFFER
    Texture2D<float> ao_buffer = ResourceDescriptorHeap[constant.ao_buffer];
    float ao = ao_buffer.Sample(linear_clamp_sampler, texcoord);
    ambient_lighting *= gtao_multi_bounce(ao, albedo);
#endif

    return float4(direct_lighting + ambient_lighting + emissive, 1.0);
}