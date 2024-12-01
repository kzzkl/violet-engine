#include "violet_common.hlsli"
#include "violet_brdf.hlsli"

ConstantBuffer<violet_camera> camera : register(b0, space0);
ConstantBuffer<violet_light> light : register(b0, space1);
ConstantBuffer<violet_mesh> mesh : register(b0, space2);

TextureCube irradiance_texture : register(t0, space3);
SamplerState irradiance_sampler : register(s0, space3);
TextureCube prefilter_texture : register(t1, space3);
SamplerState prefilter_sampler : register(s1, space3);
Texture2D brdf_lut : register(t2, space3);
SamplerState brdf_lut_sampler : register(s2, space3);
struct pbr_material
{
    float3 albedo;
    float roughness;
    float metallic;
};
ConstantBuffer<pbr_material> material : register(b3, space3);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
};

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.world_position = mul(mesh.model, float4(input.position, 1.0)).xyz;
    output.position = mul(camera.view_projection, float4(output.world_position, 1.0));
    output.normal = mul((float3x3)mesh.model, input.normal);

    return output;
}

float4 fs_main(vs_out input) : SV_TARGET
{
    float3 albedo = material.albedo;
    float roughness = material.roughness;
    float metallic = material.metallic;

    float3 n = normalize(input.normal);
    float3 v = normalize(camera.position - input.world_position);
    float3 r = reflect(-v, n);

    float3 f0 = float3(0.04, 0.04, 0.04);
    f0 = lerp(f0, albedo, metallic);

    float3 light_out = float3(0.0, 0.0, 0.0);
    for (uint i = 0; i < light.directional_light_count; ++i)
    {
        float3 radiance = light.directional_lights[i].color;

        float3 l = normalize(-light.directional_lights[i].direction);
        float3 h = normalize(v + l);
        
        float ndf = distribution_ggx(n, h, roughness);
        float g = geometry_smith(n, v, l, roughness);
        float3 f = fresnel_schlick(max(dot(h, v), 0.0), f0);

        float3 numerator = ndf * g * f;
        float denominator = 4.0 * max(dot(n, v), 0.0) * max(dot(n, l), 0.0) + 0.0001;
        float3 specular = numerator / denominator;

        float3 ks = f0;
        float3 kd = 1.0 - ks;
        kd *= 1.0 - metallic;

        float NdotL = max(dot(n, l), 0.0);

        light_out += (kd * albedo / PI + specular) * radiance * NdotL;
    }

    // ambient lighting
    float3 ambient = 0.0;
    {
        float3 f = fresnel_schlick_roughness(max(dot(n, v), 0.0), f0, roughness);
        float3 ks = f;
        float3 kd = 1.0 - ks;
        kd *= 1.0 - metallic;

        float3 irradiance = irradiance_texture.Sample(irradiance_sampler, n).rgb;
        float3 diffuse = albedo * irradiance;

        const float max_lod = 4.0;
        float3 prefilter = prefilter_texture.SampleLevel(prefilter_sampler, r, roughness * max_lod).rgb;
        float2 brdf = brdf_lut.Sample(brdf_lut_sampler, float2(max(dot(n, v), 0.0), roughness)).rg;
        float3 specular = prefilter * (f * brdf.x + brdf.y);

        ambient = kd * diffuse + specular;
    }
    
    float3 color = light_out + ambient;

    return float4(color, 1.0);
}