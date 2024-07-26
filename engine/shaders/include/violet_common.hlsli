#ifndef VIOLET_COMMON_INCLUDE
#define VIOLET_COMMON_INCLUDE

struct violet_camera
{
    float4x4 view;
    float4x4 project;
    float4x4 view_projection;
    float3 position;
    uint padding;
};

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

struct violet_mesh
{
	float4x4 model;
	float4x4 normal;
};

#endif