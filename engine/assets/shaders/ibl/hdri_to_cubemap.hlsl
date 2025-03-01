#include "common.hlsli"

struct constant_data
{
    uint width;
    uint height;
    uint env_map;
    uint cube_map;
};
PushConstant(constant_data, constant);

static const float3 forward_dir[6] = {
    float3(1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, -1.0f),
};

static const float3 up_dir[6] = {
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
};

static const float3 right_dir[6] = {
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
};

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    float2 offset = float2(dtid.xy) / float2(constant.width, constant.height) * 2.0 - 1.0;
    offset.y = -offset.y;

    float3 N = normalize(forward_dir[dtid.z] + offset.x * right_dir[dtid.z] + offset.y * up_dir[dtid.z]);

    const float2 inv_atan = {0.1591, -0.3183};
    float2 texcoord = float2(atan2(N.z, N.x), asin(N.y));
    texcoord *= inv_atan;
    texcoord += 0.5;

    Texture2D<float4> env_map = ResourceDescriptorHeap[constant.env_map];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    RWTexture2DArray<float3> cube_map = ResourceDescriptorHeap[constant.cube_map];
    cube_map[dtid] = env_map.SampleLevel(linear_clamp_sampler, texcoord, 0).rgb;
}