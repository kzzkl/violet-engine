#include "brdf.hlsli"

struct constant_data
{
    uint width;
    uint height;
    uint cube_map;
    uint prefilter_map;
    float roughness;
    uint resolution;
    uint padding0;
    uint padding1;
};
PushConstant(constant_data, constant);

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
    float2 offset = float2(dtid.xy + 0.5) / float2(constant.width, constant.height) * 2.0 - 1.0;
    offset.y = -offset.y;

    float3 N = normalize(forward_dir[dtid.z] + offset.x * right_dir[dtid.z] + offset.y * up_dir[dtid.z]);

    RWTexture2DArray<float3> prefilter_map = ResourceDescriptorHeap[constant.prefilter_map];
    TextureCube<float3> cube_map = ResourceDescriptorHeap[constant.cube_map];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float3 V = N;

    const uint sample_count = 64;
    float weight = 0.0;
    float3 result = float3(0.0, 0.0, 0.0);
    for (uint i = 0; i < sample_count; ++i)
    {
        float2 xi = hammersley(i, sample_count);
        float3 H = importance_sample_ggx(xi, N, constant.roughness);
        float3 L = reflect(-V, H);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            float NdotH = dot(N, H);
            float HdotV = dot(H, V);

            float D = d_ggx(NdotH, constant.roughness);
            float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001; 

            float sa_texel  = 4.0 * PI / (6.0 * constant.resolution * constant.resolution);
            float sa_sample = 1.0 / (float(sample_count) * pdf + 0.0001);

            float mip_level = constant.roughness == 0.0 ? 0.0 : 0.5 * log2(sa_sample / sa_texel); 

            result += cube_map.SampleLevel(linear_clamp_sampler, L, mip_level) * NdotL;
            weight += NdotL;
        }
    }
    result = result / weight;

    prefilter_map[dtid] = result;
}