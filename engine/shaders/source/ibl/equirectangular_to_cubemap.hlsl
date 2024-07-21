Texture2D env_texture : register(t0, space0);
SamplerState env_sampler : register(s0, space0);

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

float4 sample_spherical_map(float3 normal)
{
    const float2 inv_atan = {0.1591, -0.3183};
    float2 uv = float2(atan2(normal.z, normal.x), asin(normal.y));
    uv *= inv_atan;
    uv += 0.5;

    return env_texture.Sample(env_sampler, uv);
}

fs_out fs_main(vs_out input)
{
    fs_out output;
    output.right = sample_spherical_map(normalize(input.right));
    output.left = sample_spherical_map(normalize(input.left));
    output.top = sample_spherical_map(normalize(input.top));
    output.bottom = sample_spherical_map(normalize(input.bottom));
    output.front = sample_spherical_map(normalize(input.front));
    output.back = sample_spherical_map(normalize(input.back));

    return output;
}