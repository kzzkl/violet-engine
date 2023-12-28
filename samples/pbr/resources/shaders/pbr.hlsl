#include "violet_mesh.hlsl"
#include "violet_camera.hlsl"
#include "violet_light.hlsl"

struct vs_in
{
    [[vk::location(0)]] float3 position: POSITION;
    [[vk::location(1)]] float3 normal: NORMAL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_normal: WORLD_NORMAL;
    float3 world_position: WORLD_POSITION;
};

ConstantBuffer<violet_mesh> mesh : register(b0, space0);

struct pbr_material
{
    float3 albedo;
    float metalness;
    float roughness;
};
ConstantBuffer<pbr_material> material : register(b0, space1);

ConstantBuffer<violet_camera> camera : register(b0, space2);
TextureCube sky_texture : register(t1, space2);
SamplerState sky_sampler : register(s1, space2);

ConstantBuffer<violet_light> light : register(b0, space3);

vs_out vs_main(vs_in input)
{
    vs_out output;

    float4 position = mul(mesh.model, float4(input.position, 1.0));

    output.world_position = position.xyz;
    output.position = mul(camera.view_projection, position);
    output.world_normal = mul(input.normal, (float3x3)mesh.model);

    return output;
}

static const float PI = 3.141592;
static const float Epsilon = 0.00001;

float ndf_ggx(float Lh_cos, float roughness)
{
    float alpha = roughness * roughness;
    float alpha_sq = alpha * alpha;

    float d = (Lh_cos * Lh_cos) * (alpha_sq - 1.0) + 1.0;

    return alpha_sq / (PI * d * d);
}

float3 fresnel_schlick(float3 F0, float cos)
{
    return F0 + (1.0 - F0) * pow(1.0 - cos, 5.0);
}

float geometry_schlick_ggx(float Lo_cos, float Li_cos, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    
    float Gi = Li_cos / (Li_cos * (1.0 - k) + k);
    float Go = Lo_cos / (Lo_cos * (1.0 - k) + k);

    return Gi * Go;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    float3 Lo = normalize(camera.position - input.world_position);
    float3 N = normalize(input.world_normal);
    float Lo_cos = max(0.0, dot(N, Lo));
    float3 Lr = 2.0 * Lo_cos * N - Lo;

    float3 F0 = lerp(0.04, material.albedo, material.metalness);

    float3 direct_light = 0.0;
    for (uint i = 0; i < light.directional_light_count; ++i)
    {
        float3 Li = -light.directional_lights[i].direction;
        float3 radiance = light.directional_lights[i].color;

        float3 Lh = normalize(Li + Lo);
        float Lh_cos = max(0.0, dot(Lh, N));

        float Li_cos = max(0.0, dot(N, Li));

        float D = ndf_ggx(Lh_cos, material.roughness);
        float3 F = fresnel_schlick(F0, max(0.0, dot(Lo, Lh)));
        float G = geometry_schlick_ggx(Lo_cos, Li_cos, material.roughness);

        float3 kd = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), material.metalness);
        float3 diffuse_brdf = kd * material.albedo / PI;
        float3 specular_brdf = (D * F * G) / max(Epsilon, 4.0 * Li_cos * Lo_cos);

        direct_light += (diffuse_brdf + specular_brdf) * radiance * Li_cos;
    }

    float3 ambient_light = 0.0;
    {
        float3 irradiance = sky_texture.Sample(sky_sampler, N).rgb;
        float3 F = fresnel_schlick(F0, Lo_cos);
        float3 kd = lerp(1.0 - F, 0.0, material.metalness);
        float3 diffuse_ibl = kd * material.albedo * irradiance;

        ambient_light = diffuse_ibl;
    }

    // return float4(direct_light + ambient_light, 1.0);
    return float4(sky_texture.Sample(sky_sampler, Lr).rgb * 0.5 + direct_light, 1.0);
}