#ifndef GBUFFER_HLSLI
#define GBUFFER_HLSLI

struct gbuffer_data
{
    float4 albedo : SV_TARGET0;
};

gbuffer_data gbuffer_encode(float3 albedo)
{
    gbuffer_data data;

    data.albedo = float4(albedo, 1.0);

    return data;
}

void gbuffer_decode(gbuffer_data data, out float3 albedo)
{
    albedo = data.albedo.rgb;
}

#endif