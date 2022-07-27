struct ash_directional_light_data
{
    float3 direction;
    float _padding_0;
    float3 color;
    float _padding_1;

    float4x4 light_vp;
};

struct ash_light
{
    ash_directional_light_data directional_light[4];
    uint directional_light_count;
};