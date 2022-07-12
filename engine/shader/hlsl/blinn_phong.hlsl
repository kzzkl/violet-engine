cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
};

cbuffer ash_blinn_phong_material : register(b0, space1)
{
    float3 diffuse;
    float3 fresnel;
    float roughness;
};

cbuffer ash_camera : register(b0, space2)
{
    float3 camera_position;
    float3 camera_direction;

    float4x4 transform_v;
    float4x4 transform_p;
    float4x4 transform_vp;
};

struct ash_directional_light_data
{
    float3 direction;
    float _padding_0;
    float3 color;
    float _padding_1;
};

cbuffer ash_light : register(b0, space3)
{
    ash_directional_light_data directional_light[4];
    uint directional_light_count;
};

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

vs_out vs_main(vs_in vin)
{
    vs_out result;

    float4 world_position = mul(mul(float4(vin.position, 1.0f), transform_m), transform_v);
    result.world_position = world_position.xyz;
    result.position = mul(world_position, transform_p);
    result.normal = mul(float4(vin.normal, 0.0f), transform_m).xyz;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    float3 ambient_light = float3(0.1f, 0.1f, 0.1f);

    float3 normal = normalize(pin.normal);
    float3 view_direction = normalize(camera_position - pin.world_position);

    // ambient
    float3 color = diffuse * ambient_light;

    for (uint i = 0; i < directional_light_count; ++i)
    {
        float3 half_direction = normalize(directional_light[i].direction + view_direction);
        float incident_cos = dot(normal, directional_light[i].direction);

        float3 light_strength = max(0.0f, incident_cos) * directional_light[i].color;

        // diffuse
        color += light_strength * diffuse;

        // specular
        float m = (1.0f - roughness) * 256.0f;
        float roughness_factor = (m + 8.0f) * pow(max(dot(half_direction, normal), 0.0f), m) / 8.0f;

        float f0 = 1.0f - saturate(incident_cos);
        float3 fresnel_factor = fresnel + (1.0f - fresnel) * (f0 * f0 * f0 * f0 * f0);

        float3 spec_albedo = fresnel_factor * roughness_factor;
        spec_albedo = spec_albedo / (spec_albedo + 1.0f);

        color += light_strength * spec_albedo;
    }

    return float4(color, 1.0f);
}