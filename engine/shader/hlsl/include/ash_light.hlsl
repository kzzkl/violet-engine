struct ash_directional_light_data
{
    float3 direction;
    bool shadow;
    float3 color;
    uint shadow_index;
};

#define ASH_MAX_SHADOW_COUNT 4
#define ASH_MAX_CASCADED_COUNT 4

struct ash_light
{
    ash_directional_light_data directional_light[4];
    uint directional_light_count;
    float3 ambient_light;

    float4x4 shadow_v[ASH_MAX_SHADOW_COUNT];
    float4 cascade_depths;
    float4 cascade_scale[ASH_MAX_SHADOW_COUNT][ASH_MAX_CASCADED_COUNT];
    float4 cascade_offset[ASH_MAX_SHADOW_COUNT][ASH_MAX_CASCADED_COUNT];

    uint shadow_count;
    uint cascade_count;
};

uint shadow_cascade_index(float camera_depth, ash_light light)
{
    if (camera_depth < light.cascade_depths[0])
        return 0;
    else if (camera_depth < light.cascade_depths[1])
        return 1;
    else if (camera_depth < light.cascade_depths[2])
        return 2;
    else
        return 3;
}

float shadow_pcf(
    Texture2D<float> shadow_map,
    SamplerComparisonState shadow_map_sampler,
    float4 shadow_position)
{
    float2 uv = float2(shadow_position.x * 0.5f + 0.5f, shadow_position.y * -0.5f + 0.5f);

    uint width, height, mip_count;
    shadow_map.GetDimensions(0, width, height, mip_count);

    float dx = 1.0f / (float)width;
    const float2 offsets[9] =
    {
        float2(-dx,  -dx), float2(0.0f,  -dx), float2(dx,  -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx,  +dx), float2(0.0f,  +dx), float2(dx,  +dx)
    };

    float result = 0.0f;
    float shadow_depth = shadow_position.z - 0.001f;
    for(int i = 0; i < 9; ++i)
        result += shadow_map.SampleCmpLevelZero(shadow_map_sampler, uv + offsets[i], shadow_depth).r;
    
    return result / 9.0f;
}