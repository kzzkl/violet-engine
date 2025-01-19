#include "brdf.hlsli"

struct constant_data
{
    uint brdf_lut;
    uint width;
    uint height;
    uint padding0;
};
ConstantBuffer<constant_data> constant : register(b0, space1);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    float NdotV = (float(dtid.x) + 0.5) / float(constant.width);
    float roughness = (float(dtid.y) + 0.5) / float(constant.height);

    float3 V = float3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

    float a = 0.0;
    float b = 0.0;

    float3 N = float3(0.0, 0.0, 1.0);

    const uint sample_count = 1024;
    for(uint i = 0; i < sample_count; ++i)
    {
        float2 xi = hammersley(i, sample_count);
        float3 H = importance_sample_ggx(xi, N, roughness);
        float3 L = reflect(-V, H);
        
        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));

        if(NdotL > 0.0)
        {
            float g = g_smith(N, V, L, roughness);
            float g_vis = (g * VdotH) / (NdotH * NdotV);
            float fc = pow(1.0 - VdotH, 5.0);

            a += (1.0 - fc) * g_vis;
            b += fc * g_vis;
        }
    }
    a /= float(sample_count);
    b /= float(sample_count);

    RWTexture2D<float2> brdf_lut = ResourceDescriptorHeap[constant.brdf_lut];
    brdf_lut[dtid.xy] = float2(a, b);
}