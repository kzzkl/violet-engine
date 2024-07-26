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

Texture2D albedo_texture : register(t3, space3);
SamplerState albedo_sampler : register(s3, space3);
Texture2D roughness_texture : register(t4, space3);
SamplerState roughness_sampler : register(s4, space3);
Texture2D metallic_texture : register(t5, space3);
SamplerState metallic_sampler : register(s5, space3);
Texture2D normal_texture : register(t6, space3);
SamplerState normal_sampler : register(s6, space3);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
};

vs_out vs_main(vs_in input)
{
    vs_out output;
    output.world_position = mul(mesh.model, float4(input.position, 1.0)).xyz;
    output.position = mul(camera.view_projection, float4(output.world_position, 1.0));
    output.normal = normalize(mul((float3x3)mesh.model, input.normal));
    output.tangent = normalize(mul((float3x3)mesh.model, input.tangent));
    output.uv = input.uv;

    return output;
}

float3 get_normal(vs_out input)
{
    float3 tangent_normal = normalize(normal_texture.Sample(normal_sampler, input.uv).xyz * 2.0 - 1.0);

    float3 n = normalize(input.normal);
    float3 t = normalize(input.tangent);
    float3 b = normalize(cross(n, t));
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, tangent_normal));
}

float4 fs_main(vs_out input) : SV_TARGET
{
    float3 albedo = albedo_texture.Sample(albedo_sampler, input.uv).rgb;
    float roughness = roughness_texture.Sample(roughness_sampler, input.uv).r;
    float metallic = metallic_texture.Sample(metallic_sampler, input.uv).r;

    float3 n = get_normal(input);

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

        float n_dot_l = max(dot(n, l), 0.0);

        light_out += (kd * albedo / PI + specular) * radiance * n_dot_l;
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