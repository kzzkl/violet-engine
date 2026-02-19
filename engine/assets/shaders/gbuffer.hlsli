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

float3 unpack_gbuffer_normal(uint gbuffer_normal, uint2 coord)
{
    Texture2D<uint> buffer = ResourceDescriptorHeap[gbuffer_normal];
    uint pack = buffer[coord];

    float2 oct = float2(float(pack >> 20) / 4095.0, float((pack & 0x000FFF00) >> 8) / 4095.0);
    return octahedron_to_normal(oct);
}

struct gbuffer
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 emissive;
    float3 normal;
    float3 position;
    uint shading_model;

    static gbuffer unpack(uint gbuffers[8], uint2 coord)
    {
        gbuffer result;

        Texture2D<float4> gbuffer_albbedo = ResourceDescriptorHeap[gbuffers[GBUFFER_ALBEDO]];
        result.albedo = gbuffer_albbedo[coord].rgb;

        Texture2D<float2> gbuffer_material = ResourceDescriptorHeap[gbuffers[GBUFFER_MATERIAL]];
        result.roughness = max(gbuffer_material[coord].x, 0.03);
        result.metallic = gbuffer_material[coord].y;

        Texture2D<uint> gbuffer_normal = ResourceDescriptorHeap[gbuffers[GBUFFER_NORMAL]];
        uint normal = gbuffer_normal[coord];
        result.shading_model = normal & 0xFF;

        float2 oct = float2(float(normal >> 20) / 4095.0, float((normal & 0x000FFF00) >> 8) / 4095.0);
        result.normal = octahedron_to_normal(oct);

        Texture2D<float4> gbuffer_emissive = ResourceDescriptorHeap[gbuffers[GBUFFER_EMISSIVE]];
        result.emissive = gbuffer_emissive[coord].rgb;

        return result;
    }

    void pack(uint gbuffers[8], uint2 coord)
    {
        RWTexture2D<float4> gbuffer_albedo = ResourceDescriptorHeap[gbuffers[GBUFFER_ALBEDO]];
        gbuffer_albedo[coord] = float4(albedo, 1.0);

        RWTexture2D<float2> gbuffer_material = ResourceDescriptorHeap[gbuffers[GBUFFER_MATERIAL]];
        gbuffer_material[coord] = float2(roughness, metallic);

        RWTexture2D<float4> gbuffer_emissive = ResourceDescriptorHeap[gbuffers[GBUFFER_EMISSIVE]];
        gbuffer_emissive[coord] = float4(emissive, 1.0);

        RWTexture2D<uint> gbuffer_normal = ResourceDescriptorHeap[gbuffers[GBUFFER_NORMAL]];
        float2 oct = normal_to_octahedron(normal);
        gbuffer_normal[coord] = (uint(oct.x * 4095.0) << 20) | (uint(oct.y * 4095.0) << 8) | shading_model;
    }
};

#endif