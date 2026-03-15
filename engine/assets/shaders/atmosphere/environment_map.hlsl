#include "atmosphere/atmosphere.hlsli"

struct constant_data
{
    uint sky_view_lut;
    uint environment_map;
    uint width;
    uint height;
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
    RWTexture2DArray<float3> environment_map = ResourceDescriptorHeap[constant.environment_map];

    float2 offset = float2(dtid.xy + 0.5) / float2(constant.width, constant.height) * 2.0 - 1.0;
    offset.y = -offset.y;

    float3 N = normalize(forward_dir[dtid.z] + offset.x * right_dir[dtid.z] + offset.y * up_dir[dtid.z]);

    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();
    
    Texture2D<float3> sky_view_lut = ResourceDescriptorHeap[constant.sky_view_lut];

    float2 uv = get_sky_view_lut_uv(N);
    float3 sky = sky_view_lut.SampleLevel(get_linear_clamp_sampler(), uv, 0.0);

    environment_map[dtid] = sky;
}