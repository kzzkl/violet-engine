#include "common.hlsli"
#include "gbuffer.hlsli"
#include "brdf.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);
struct gbuffer_data
{
    uint albedo;
    uint material;
    uint normal;
    uint depth;
    uint emissive;
};
ConstantBuffer<gbuffer_data> constant : register(b0, space3);

float4 fs_main(float2 texcoord : TEXCOORD) : SV_TARGET
{
    SamplerState point_repeat_sampler = SamplerDescriptorHeap[scene.point_repeat_sampler];
    SamplerState linear_repeat_sampler = SamplerDescriptorHeap[scene.linear_repeat_sampler];
    SamplerState linear_clamp_sampler = SamplerDescriptorHeap[scene.linear_clamp_sampler];

    gbuffer::packed packed;

    Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[constant.albedo];
    packed.albedo = gbuffer_albbedo.Sample(point_repeat_sampler, texcoord);

    Texture2D<float2> gbuffer_material = ResourceDescriptorHeap[constant.material];
    packed.material = gbuffer_material.Sample(point_repeat_sampler, texcoord);

    Texture2D<float2> gbuffer_normal = ResourceDescriptorHeap[constant.normal];
    packed.normal = gbuffer_normal.Sample(point_repeat_sampler, texcoord);

    Texture2D<float4> gbuffer_emissive = ResourceDescriptorHeap[constant.emissive];
    packed.emissive = gbuffer_emissive.Sample(point_repeat_sampler, texcoord);
    
    Texture2D<float> gbuffer_depth = ResourceDescriptorHeap[constant.depth];
    float depth = gbuffer_depth.Sample(point_repeat_sampler, texcoord);
    float3 position = gbuffer::get_position(depth, texcoord, camera.view_projection_inv);

    gbuffer::data data = gbuffer::unpack(packed);

    float3 F0 = lerp(0.04, data.albedo, data.metallic);

    float3 N = data.normal;
    float3 V = normalize(camera.position - position);

    float NdotV = saturate(dot(N, V));

    float3 direct_lighting = 0.0;
    {
        float3 light = float3(1.0, 1.0, 1.0);

        float3 L = normalize(float3(1.0, 1.0, -1.0));
        float3 H = normalize(L + V);

        float NdotL = saturate(dot(N, L));
        float NdotH = saturate(dot(N, H));
        float VdotH = saturate(dot(V, H));

        float d = d_ggx(NdotH, data.roughness);
        float vis = v_smith_joint_approx(NdotV, NdotL, data.roughness);
        float3 f = f_schlick(VdotH, F0);

        float3 specular = d * vis * f;
        float3 diffuse = data.albedo / PI * (1.0 - f);

        direct_lighting += (specular + diffuse) * NdotL * light;
    }

    float3 ambient_lighting = 0.0;
    {
        float3 R = reflect(-V, N);
        float3 f = f_schlick_roughness(NdotV, F0, data.roughness);
        float3 kd = lerp(1.0 - f, 0.0, data.metallic);

        TextureCube<float3> prefilter_map = ResourceDescriptorHeap[scene.prefilter];
        float3 prefilter = prefilter_map.SampleLevel(linear_repeat_sampler, R, data.roughness * 4.0);

        Texture2D<float2> brdf_lut = ResourceDescriptorHeap[scene.brdf_lut];
        float2 brdf = brdf_lut.Sample(linear_clamp_sampler, float2(NdotV, data.roughness));
        float3 specular = (F0 * brdf.x + brdf.y) * prefilter;

        TextureCube<float3> irradiance_map = ResourceDescriptorHeap[scene.irradiance];
        float3 irradiance = irradiance_map.Sample(linear_repeat_sampler, N);
        float3 diffuse = data.albedo * irradiance * kd / PI;

        ambient_lighting = specular + diffuse;
    }

    float3 emissive = data.emissive;

    return float4(direct_lighting + ambient_lighting + emissive, 1.0);
}