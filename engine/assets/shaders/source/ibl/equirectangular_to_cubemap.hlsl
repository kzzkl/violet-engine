struct convert_data
{
    uint width;
    uint height;
    uint env_map;
    uint cube_map;
};
ConstantBuffer<convert_data> convert : register(b0, space1);

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
    float2 offset = float2(dtid.xy) / float2(convert.width, convert.height) * 2.0 - 1.0;
    offset.y = 1.0 - offset.y;

    float3 N = normalize(forward_dir[dtid.z] + offset.x * right_dir[dtid.z] + offset.y * up_dir[dtid.z]);

    const float2 inv_atan = {0.1591, -0.3183};
    float2 uv = float2(atan2(N.z, N.x), asin(N.y));
    uv *= inv_atan;
    uv += 0.5;

    Texture2D<float4> env_map = ResourceDescriptorHeap[convert.env_map];
    SamplerState linear_sampler = SamplerDescriptorHeap[1];

    RWTexture2DArray<float4> cube_map = ResourceDescriptorHeap[convert.cube_map];
    cube_map[dtid] = env_map.SampleLevel(linear_sampler, uv, 0);
}