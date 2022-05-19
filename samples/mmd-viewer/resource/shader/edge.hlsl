[[vk::binding(0, 0)]]
cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
    float4x4 transform_mv;
    float4x4 transform_mvp;
};

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
    uint4 bone : BONE;
    float3 weight : BONE_WEIGHT;
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;
    result.position = mul(float4(vin.position, 1.0f), transform_mvp);

    float4 normal = mul(float4(vin.normal, 1.0f), transform_mv);
    float2 screen_normal = normalize(normal.xy);
    result.position.xy += screen_normal * result.position.w / 800.0f;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}