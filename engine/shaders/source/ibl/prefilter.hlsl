#include "violet_brdf.hlsli"

struct prefilter_data
{
    float roughness;
};
ConstantBuffer<prefilter_data> data : register(b0, space0);
TextureCube env_texture : register(t1, space0);
SamplerState env_sampler : register(s1, space0);

struct vs_out
{
    float4 position : SV_POSITION;
    
    float3 right : NORMAL0;
    float3 left : NORMAL1;
    float3 top : NORMAL2;
    float3 bottom : NORMAL3;
    float3 front : NORMAL4;
    float3 back : NORMAL5;
};

struct fs_out
{
    float4 right : SV_TARGET0;
    float4 left : SV_TARGET1;
    float4 top : SV_TARGET2;
    float4 bottom : SV_TARGET3;
    float4 front : SV_TARGET4;
    float4 back : SV_TARGET5;
};

float4 prefilter(float3 normal)
{
    float3 view = normal;

    const uint sample_count = 1024u;
    float weight = 0.0;   
    float3 result = float3(0.0, 0.0, 0.0);     
    for(uint i = 0; i < sample_count; ++i)
    {
        float2 xi = hammersley(i, sample_count);
        float3 half_vec = importance_sample_ggx(xi, normal, data.roughness);
        float3 light = normalize(2.0 * dot(view, half_vec) * half_vec - view);

        float n_dot_l = max(dot(normal, light), 0.0);
        if(n_dot_l > 0.0)
        {
            result += env_texture.Sample(env_sampler, light).rgb * n_dot_l;
            weight += n_dot_l;
        }
    }
    result = result / weight;

    return float4(result, 1.0);
}

fs_out fs_main(vs_out input)
{
    fs_out output;
    output.right = prefilter(normalize(input.right));
    output.left = prefilter(normalize(input.left));
    output.top = prefilter(normalize(input.top));
    output.bottom = prefilter(normalize(input.bottom));
    output.front = prefilter(normalize(input.front));
    output.back = prefilter(normalize(input.back));

    return output;
}