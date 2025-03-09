#include "mesh.hlsli"
#include "brdf.hlsli"
#include "color.hlsli"

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float3 position_ws : POSITION_WS;
    float3 normal_ws : NORMAL_WS;
    float3 tangent_ws : TANGENT_WS;
    float3 bitangent_ws : BITANGENT_WS;
    float2 texcoord : TEXCOORD;
    float4 texcoord2 : TEXCOORD2;
    uint material_address : MATERIAL_ADDRESS;
};

struct fs_output
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    float2 normal : SV_TARGET2;
    float4 emissive : SV_TARGET3;
};

float3 get_normal(vs_output input, float3 packed_normal)
{
    float3 tangent_normal = packed_normal * 2.0 - 1.0;

    float3 n = normalize(input.normal_ws);
    float3 t = normalize(input.tangent_ws);
    float3 b = normalize(cross(n, t));
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, tangent_normal));
}

float3 direct_light(float3 N, float3 V, float3 albedo, float roughness, float metallic, uint ramp_texture)
{
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    Texture2D<float4> ramp = ResourceDescriptorHeap[ramp_texture];

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];

    float NdotV = saturate(dot(N, V));
    float3 F0 = lerp(0.04, albedo, metallic);

    float3 direct_lighting = 0.0;
    if (scene.light_count > 0)
    {
        light_data light = lights[0];

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
        
        float3 diffuse_ramp = ramp.Sample(linear_clamp_sampler, float2(NdotL, 0.875)).rgb;
        float3 specular_ramp = ramp.Sample(linear_clamp_sampler, float2(NdotL, 0.625)).rgb;

        direct_lighting += (specular * specular_ramp + diffuse * diffuse_ramp) * NdotL * light.color;
    }

    return direct_lighting;
}

float3 indirect_light(float3 N, float3 V, float3 albedo, float roughness, float metallic, uint brdf_lut_address)
{
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float NdotV = saturate(dot(N, V));
    float3 F0 = lerp(0.04, albedo, metallic);

    float3 R = reflect(-V, N);
    float3 f = f_schlick_roughness(NdotV, F0, roughness);
    float3 kd = lerp(1.0 - f, 0.0, metallic);

    TextureCube<float3> prefilter_map = ResourceDescriptorHeap[scene.prefilter];
    float3 prefilter = prefilter_map.SampleLevel(linear_clamp_sampler, R, roughness * 4.0);

    Texture2D<float2> brdf_lut = ResourceDescriptorHeap[brdf_lut_address];
    float2 brdf = brdf_lut.Sample(linear_clamp_sampler, float2(NdotV, roughness));
    float3 specular = (F0 * brdf.x + brdf.y) * prefilter;

    TextureCube<float3> irradiance_map = ResourceDescriptorHeap[scene.irradiance];
    float3 irradiance = irradiance_map.Sample(linear_clamp_sampler, N);
    float3 diffuse = albedo * irradiance * kd / PI;

    return specular + diffuse;
}