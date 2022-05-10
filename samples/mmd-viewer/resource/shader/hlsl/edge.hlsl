cbuffer mmd_skeleton : register(b0)
{
    float4x4 offset[512];
};

cbuffer ash_pass : register(b1)
{
    float4 camera_position;
    float4 camera_direction;

    float4x4 transform_v;
    float4x4 transform_p;
    float4x4 transform_vp;
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
    float weights[4] = { 0.0, 0.0, 0.0, 0.0 };
    weights[0] = vin.weight.x;
    weights[1] = vin.weight.y;
    weights[2] = vin.weight.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float4x4 m;
    for (int i = 0; i < 4; ++i)
        m += weights[i] * offset[vin.bone[i]];

    float4x4 mv = mul(m, transform_v);
    float4x4 mvp = mul(m, transform_vp);

    vs_out result;
    result.position = mul(float4(vin.position, 1.0f), mvp);

    float4 normal = mul(float4(vin.normal, 1.0f), mv);
    float2 screen_normal = normalize(normal.xy);
    result.position.xy += screen_normal * result.position.w / 800.0f;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}