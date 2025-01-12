#include "common.hlsli"

struct irradiance_data
{
    uint width;
    uint height;
    uint cube_map;
    uint irradiance_map;
};
ConstantBuffer<irradiance_data> irradiance : register(b0, space1);

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
    float2 offset = float2(dtid.xy) / float2(irradiance.width, irradiance.height) * 2.0 - 1.0;
    offset.y = -offset.y;

    float3 N = normalize(forward_dir[dtid.z] + offset.x * right_dir[dtid.z] + offset.y * up_dir[dtid.z]);

    TextureCube<float3> cube_map = ResourceDescriptorHeap[irradiance.cube_map];
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);

    float3 color = 0.0;
    
    float sample_delta = 0.025;
    uint sample_count = 0;
    for (float phi = 0.0; phi < TWO_PI; phi += sample_delta)
    {
        for (float theta = 0.0; theta < HALF_PI; theta += sample_delta)
        {
            float3 tangent = float3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            float3 texcoord = normalize(tangent.x * right + tangent.y * up + tangent.z * N);
            color += cube_map.SampleLevel(linear_clamp_sampler, texcoord, 0.0) * cos(theta) * sin(theta);
            ++sample_count;
        }
    }

    RWTexture2DArray<float3> irradiance_map = ResourceDescriptorHeap[irradiance.irradiance_map];
    irradiance_map[dtid] = PI * color / float(sample_count);
}