#ifndef SURFACE_HLSLI
#define SURFACE_HLSLI

struct surface
{
    float3 albedo;
    float opacity;
    float roughness;
    float metallic;
    float3 emissive;

    float3 position_ws;
    float3 normal_ws;
};

#endif