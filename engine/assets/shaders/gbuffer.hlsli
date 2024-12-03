#ifndef GBUFFER_HLSLI
#define GBUFFER_HLSLI

struct gbuffer_packed
{
    float4 albedo : SV_TARGET0;
};

gbuffer_packed gbuffer_encode(float3 albedo)
{
    gbuffer_packed data;

    data.albedo = float4(albedo, 1.0);

    return data;
}

void gbuffer_decode(gbuffer_packed data, out float3 albedo)
{
    albedo = data.albedo.rgb;
}

#endif