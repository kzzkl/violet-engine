#ifndef GBUFFER_HLSLI
#define GBUFFER_HLSLI

namespace gbuffer
{
float2 normal_to_octahedron(float3 N)
{
    N.xy /= dot(1, abs(N));
    if (N.z <= 0)
    {
        N.xy = (1 - abs(N.yx)) * select(N.xy >= 0, float2(1, 1), float2(-1, -1));
    }
    N.xy = N.xy * 0.5 + 0.5;
    return N.xy;
}

float3 octahedron_to_normal(float2 oct)
{
    oct = oct * 2.0 - 1.0;
    float3 N = float3(oct, 1 - dot(1, abs(oct)));
    if (N.z < 0)
    {
        N.xy = (1 - abs(N.yx)) * select(N.xy >= 0, float2(1, 1), float2(-1, -1));
    }
    return normalize(N);
}

float3 get_position(float depth, float2 texcoord, float4x4 view_projection_inv)
{
    texcoord.y = 1.0 - texcoord.y;
    float4 ndc = float4(texcoord * 2.0 - 1.0, depth, 1.0);
    float4 world_position = mul(view_projection_inv, ndc);

    return world_position.xyz / world_position.w;
}

struct data
{
    float3 albedo;
    float roughness;
    float metallic;
    float3 normal;
    float3 emissive;
};

struct packed
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    float2 normal : SV_TARGET2;
    float4 emissive : SV_TARGET3;
};

packed pack(data gbuffer)
{
    packed gbuffer_packed;

    gbuffer_packed.albedo = float4(gbuffer.albedo, 1.0);
    gbuffer_packed.material = float2(gbuffer.roughness, gbuffer.metallic);
    gbuffer_packed.normal = normal_to_octahedron(gbuffer.normal);
    gbuffer_packed.emissive = float4(gbuffer.emissive, 1.0);

    return gbuffer_packed;
}

data unpack(packed gbuffer_packed)
{
    data gbuffer;

    gbuffer.albedo = gbuffer_packed.albedo.rgb;
    gbuffer.roughness = gbuffer_packed.material.x;
    gbuffer.metallic = gbuffer_packed.material.y;
    gbuffer.normal = octahedron_to_normal(gbuffer_packed.normal);
    gbuffer.emissive = gbuffer_packed.emissive.rgb;

    return gbuffer;
}
} // namespace gbuffer

#endif