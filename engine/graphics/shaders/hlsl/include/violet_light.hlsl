struct violet_directional_light
{
    float3 direction;
    bool shadow;
    float3 color;
    uint padding;
};

struct violet_light
{
    violet_directional_light directional_lights[16];
    uint directional_light_count;
    uint padding1;
    uint padding2;
    uint padding3;
};