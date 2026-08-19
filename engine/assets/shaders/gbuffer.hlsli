#ifndef GBUFFER_HLSLI
#define GBUFFER_HLSLI

#include "common.hlsli"

static const uint GBUFFER_ALBEDO = 0;
static const uint GBUFFER_MATERIAL = 1;
static const uint GBUFFER_NORMAL = 2;
static const uint GBUFFER_EMISSIVE = 3;

float2 normal_to_octahedron(float3 N)
{
    N.xy /= dot(1, abs(N));
    if (N.z <= 0)
    {
        N.xy = (1 - abs(N.yx)) * select(N.xy >= 0, float2(1, 1), float2(-1, -1));
    }
    N.xy = N.xy * 0.5 + 0.5;
    return N.xy;
}

float3 octahedron_to_normal(float2 oct)
{
    oct = oct * 2.0 - 1.0;
    float3 N = float3(oct, 1 - dot(1, abs(oct)));
    if (N.z < 0)
    {
        N.xy = (1 - abs(N.yx)) * select(N.xy >= 0, float2(1, 1), float2(-1, -1));
    }
    return normalize(N);
}

float3 unpack_gbuffer_normal(Texture2D<uint> gbuffer_normal, uint2 coord)
{
    uint pack = gbuffer_normal[coord];

    float2 oct = float2(float(pack >> 20) / 4095.0, float((pack & 0x000FFF00) >> 8) / 4095.0);
    return octahedron_to_normal(oct);
}

float3 unpack_gbuffer_normal(uint gbuffer_normal, uint2 coord)
{
    Texture2D<uint> buffer = ResourceDescriptorHeap[gbuffer_normal];
    return unpack_gbuffer_normal(buffer, coord);
}

struct gbuffer
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 emissive;
    float3 normal;
    uint shading_model;

    static gbuffer unpack(float4 gbuffer_albedo, float2 gbuffer_material, uint gbuffer_normal, float3 gbuffer_emissive)
    {
        gbuffer result;

        result.albedo = gbuffer_albedo.rgb;
        result.roughness = max(gbuffer_material.x, 0.03);
        result.metallic = gbuffer_material.y;

        uint normal = gbuffer_normal;
        result.shading_model = normal & 0xFF;

        float2 oct = float2(float(normal >> 20) / 4095.0, float((normal & 0x000FFF00) >> 8) / 4095.0);
        result.normal = octahedron_to_normal(oct);

        result.emissive = gbuffer_emissive;

        return result;
    }

    void pack(out float4 gbuffer_albedo, out float2 gbuffer_material, out uint gbuffer_normal, out float3 gbuffer_emissive)
    {
        gbuffer_albedo = float4(albedo, 1.0);
        gbuffer_material = float2(roughness, metallic);
        gbuffer_emissive = emissive;
        float2 oct = normal_to_octahedron(normal);
        gbuffer_normal = (uint(oct.x * 4095.0) << 20) | (uint(oct.y * 4095.0) << 8) | shading_model;
    }
};

#endif