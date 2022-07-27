#include "ash_mvp.hlsl"
#include "ash_light.hlsl"

ConstantBuffer<ash_object> object : register(b0, space0);

struct ash_blinn_phong_material
{
    float3 diffuse;
    float _padding_0;
    float3 fresnel;
    float roughness;
};
ConstantBuffer<ash_blinn_phong_material> material : register(b0, space1);

ConstantBuffer<ash_camera> camera : register(b0, space2);
ConstantBuffer<ash_light> light : register(b0, space3);

Texture2D<float> shadow_map : register(t0, space3);
SamplerComparisonState shadow_sampler : register(s6);
SamplerState shadow_sampler_debug : register(s6);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_position : W_POSITION;
    float4 shadow_position : S_POSITION;
    float3 normal : NORMAL;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    float4 world_position = mul(float4(vin.position, 1.0f), object.transform_m);
    result.world_position = world_position.xyz;

    result.shadow_position = mul(world_position, light.directional_light[0].light_vp);

    result.position = mul(world_position, camera.transform_vp);
    result.normal = mul(vin.normal, (float3x3)object.transform_m);

    return result;
}

float calculate_shadow_factor(float4 shadow_position)
{
    shadow_position.xyz /= shadow_position.w;
    float2 uv = float2(shadow_position.x * 0.5f + 0.5f, shadow_position.y * -0.5f + 0.5f);

    // Depth in NDC space.
    float depth = shadow_position.z - 0.0005f;

    uint width, height, mip_count;
    shadow_map.GetDimensions(0, width, height, mip_count);

    // return shadow_map.SampleCmpLevelZero(shadow_sampler, uv, depth).r;

    // Texel size.
    float dx = 1.0f / (float)width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    for(int i = 0; i < 9; ++i)
    {
        percentLit += shadow_map.SampleCmpLevelZero(shadow_sampler, uv + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}

/*float3 blinn_phong(vs_out pin, ash_directional_light_data light)
{
    float3 color = diffuse;

    float3 ambient = ambient_light * color;

}*/

float4 ps_main(vs_out pin) : SV_TARGET
{
    float3 ambient_light = float3(0.3f, 0.3f, 0.3f);

    float3 normal = normalize(pin.normal);
    float3 view_direction = normalize(camera.position - pin.world_position);

    // ambient
    float3 color = material.diffuse * ambient_light;

    for (uint i = 0; i < light.directional_light_count; ++i)
    {
        float3 light_direction = -light.directional_light[i].direction;
        float3 half_direction = normalize(light_direction + view_direction);
        float incident_cos = dot(normal, light_direction);

        float3 light_strength = max(0.0f, incident_cos) * light.directional_light[i].color;
        light_strength = light_strength  * calculate_shadow_factor(pin.shadow_position);

        // diffuse
        color += light_strength * material.diffuse;

        // specular
        float m = (1.0f - material.roughness) * 256.0f;
        float roughness_factor = (m + 8.0f) * pow(max(dot(half_direction, normal), 0.0f), m) / 8.0f;

        float f0 = 1.0f - saturate(incident_cos);
        float3 fresnel_factor = material.fresnel + (1.0f - material.fresnel) * (f0 * f0 * f0 * f0 * f0);

        float3 spec_albedo = fresnel_factor * roughness_factor;
        spec_albedo = spec_albedo / (spec_albedo + 1.0f);

        color += light_strength * spec_albedo;
    }

    return float4(color, 1.0f);
}