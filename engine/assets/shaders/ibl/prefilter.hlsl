#include "brdf.hlsli"

struct prefilter_data
{
    uint width;
    uint height;
    uint cube_map;
    uint prefilter_map;
    float roughness;
    uint level;
    uint padding0;
    uint padding1;
};
ConstantBuffer<prefilter_data> prefilter : register(b0, space1);

static const float3 forward_dir[6] = {
    float3(1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, -1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, -1.0),
};

static const float3 up_dir[6] = {
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 1.0, 0.0),
};

static const float3 right_dir[6] = {
    float3(0.0, 0.0, -1.0),
    float3(0.0, 0.0, 1.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(1.0, 0.0, 0.0),
    float3(-1.0, 0.0, 0.0),
};

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    float2 offset = float2(dtid.xy) / float2(prefilter.width, prefilter.height) * 2.0 - 1.0;
    offset.y = -offset.y;

    float3 N = normalize(forward_dir[dtid.z] + offset.x * right_dir[dtid.z] + offset.y * up_dir[dtid.z]);

    RWTexture2DArray<float3> prefilter_map = ResourceDescriptorHeap[prefilter.prefilter_map];
    TextureCube<float3> cube_map = ResourceDescriptorHeap[prefilter.cube_map];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    if (prefilter.level == 0)
    {
        prefilter_map[dtid] = cube_map.SampleLevel(linear_clamp_sampler, N, 0);
    }
    else
    {
        float3 V = N;

        const uint sample_count = 64;
        float weight = 0.0;
        float3 result = float3(0.0, 0.0, 0.0);
        for (uint i = 0; i < sample_count; ++i)
        {
            float2 xi = hammersley(i, sample_count);
            float3 H = importance_sample_ggx(xi, N, prefilter.roughness);
            float3 L = reflect(-V, H);

            float NdotL = max(dot(N, L), 0.0);
            if (NdotL > 0.0)
            {
                result += cube_map.SampleLevel(linear_clamp_sampler, L, 0) * NdotL;
                weight += NdotL;
            }
        }
        result = result / weight;

        prefilter_map[dtid] = result;
    }
}