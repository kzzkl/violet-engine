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

    float4x4 shadow_v[ASH_MAX_SHADOW_COUNT];
    float4 cascaded_scale[ASH_MAX_SHADOW_COUNT][ASH_MAX_CASCADED_COUNT];
    float4 cascaded_offset[ASH_MAX_SHADOW_COUNT][ASH_MAX_CASCADED_COUNT];

    uint directional_light_count;
    uint shadow_count;
    uint cascaded_count;
};