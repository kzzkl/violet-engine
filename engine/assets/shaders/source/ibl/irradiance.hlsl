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
    float2 offset = float2(dtid.xy) / float2(irradiance.width, irradiance.height) * 2.0 - 1.0;
    offset.y = 1.0 - offset.y;

    float3 N = normalize(forward_dir[dtid.z] + offset.x * right_dir[dtid.z] + offset.y * up_dir[dtid.z]);

    TextureCube<float4> cube_map = ResourceDescriptorHeap[irradiance.cube_map];
    SamplerState linear_sampler = SamplerDescriptorHeap[1];

    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = normalize(cross(up, N));
    up = cross(N, right);

    float delta_phi = TWO_PI / irradiance.width;
    float delta_theta = HALF_PI / irradiance.height;

    float3 color = float3(0.0, 0.0, 0.0);
    uint sample_count = 0;
    for (float phi = 0.0; phi < TWO_PI; phi += delta_phi)
    {
        for (float theta = 0.0; theta < HALF_PI; theta += delta_theta)
        {
            float3 temp = cos(phi) * right + sin(phi) * up;
            float3 uvw = cos(theta) * N + sin(theta) * temp;
            color += cube_map.SampleLevel(linear_sampler, uvw, 0).rgb * cos(theta) * sin(theta);
            ++sample_count;
        }
    }

    RWTexture2DArray<float4> irradiance_map = ResourceDescriptorHeap[irradiance.irradiance_map];
    irradiance_map[dtid] = float4(PI * color / float(sample_count), 1.0);
}