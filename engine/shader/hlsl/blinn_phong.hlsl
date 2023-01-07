#include "violet_mvp.hlsl"
#include "violet_light.hlsl"

ConstantBuffer<violet_object> object : register(b0, space0);

struct violet_blinn_phong_material
{
    float3 diffuse;
    float _padding_0;
    float3 fresnel;
    float roughness;
};
ConstantBuffer<violet_blinn_phong_material> material : register(b0, space1);

ConstantBuffer<violet_camera> camera : register(b0, space2);

ConstantBuffer<violet_light> light : register(b0, space3);
Texture2D<float> shadow_map[16] : register(t0, space3);
SamplerComparisonState shadow_sampler : register(s6);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_position : WORLD_POSITION;
    float3 normal : NORMAL;

    float4 shadow_position[VIOLET_MAX_SHADOW_COUNT] : SHADOW_POSITION;
    float camera_depth : CAMERA_DEPTH;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    float4 world_position = mul(float4(vin.position, 1.0f), object.transform_m);
    result.world_position = world_position.xyz;
    result.position = mul(world_position, camera.transform_vp);
    result.normal = mul(vin.normal, (float3x3)object.transform_m);

    for (uint i = 0 ; i < light.shadow_count; ++i)
        result.shadow_position[i] = mul(world_position, light.shadow_v[i]);
    result.camera_depth = mul(world_position, camera.transform_v).z;

    return result;
}

float shadow_factor(uint light_index, float camera_depth, float4 shadow_position)
{
    uint cascade_index = shadow_cascade_index(camera_depth, light);

    shadow_position.xyz /= shadow_position.w;
    shadow_position *= light.cascade_scale[light_index][cascade_index];
    shadow_position += light.cascade_offset[light_index][cascade_index];
    
    return shadow_pcf(shadow_map[light_index * 4 + cascade_index], shadow_sampler, shadow_position);
}

float3 blinn_phong(vs_out pin)
{
    float3 color = material.diffuse;

    float3 normal = normalize(pin.normal);
    float3 view_direction = normalize(camera.position - pin.world_position);

    // ambient
    color = color * light.ambient_light;

    for (uint i = 0; i < light.directional_light_count; ++i)
    {
        float3 light_direction = -light.directional_light[i].direction;
        float3 half_direction = normalize(light_direction + view_direction);
        float incident_cos = dot(normal, light_direction);

        float3 light_strength = max(0.0f, incident_cos) * light.directional_light[i].color;
        light_strength = light_strength  * shadow_factor(i, pin.camera_depth, pin.shadow_position[i]);

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

    return color;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    float4 cascade_color;
    if (pin.camera_depth < light.cascade_depths[0])
        cascade_color = float4(1.0f, 0.5f, 0.5f, 1.0f);
    else if (pin.camera_depth < light.cascade_depths[1])
        cascade_color = float4(1.0f, 1.0, 0.5f, 1.0f);
    else if (pin.camera_depth < light.cascade_depths[2])
        cascade_color = float4(1.0f, 0.5f, 1.0f, 1.0f);
    else
        cascade_color = float4(0.5f, 0.5f, 1.0f, 1.0f);

    float3 color = blinn_phong(pin);

    return float4(color, 1.0f) * cascade_color;
}