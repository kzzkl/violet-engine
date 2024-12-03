#include "brdf.hlsli"

struct vs_out
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

float2 fs_main(vs_out input) : SV_Target
{
    float NdotV = input.uv.x;
    float roughness = input.uv.y;

    float3 view = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

    float a = 0.0;
    float b = 0.0;

    float3 normal = float3(0.0, 0.0, 1.0);

    const uint sample_count = 1024;
    for(uint i = 0; i < sample_count; ++i)
    {
        float2 xi = hammersley(i, sample_count);
        float3 half_vec = importance_sample_ggx(xi, normal, roughness);
        float3 light  = normalize(2.0 * dot(view, half_vec) * half_vec - view);

        float NdotL = max(light.z, 0.0);
        float NdotH = max(half_vec.z, 0.0);
        float VdotH = max(dot(view, half_vec), 0.0);

        if(NdotL > 0.0)
        {
            float g = geometry_smith(normal, view, light, roughness);
            float g_vis = (g * VdotH) / (NdotH * NdotV);
            float fc = pow(1.0 - VdotH, 5.0);

            a += (1.0 - fc) * g_vis;
            b += fc * g_vis;
        }
    }
    a /= float(sample_count);
    b /= float(sample_count);

    return float2(a, b);
}