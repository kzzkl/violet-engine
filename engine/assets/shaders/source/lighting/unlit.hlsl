#include "gbuffer.hlsli"

Texture2D gbuffer_albbedo : register(t0, space0);
SamplerState gbuffer_sampler : register(s0, space0);

float4 fs_main(float2 uv : TEXCOORD) : SV_TARGET
{
    gbuffer_data data;
    data.albedo = gbuffer_albbedo.Sample(gbuffer_sampler, uv);

    float3 albedo;
    gbuffer_decode(data, albedo);

    return float4(albedo, 1.0);
}