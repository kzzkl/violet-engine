cbuffer mmd_skeleton : register(b0, space0)
{
    float4x4 offset[512];
};

StructuredBuffer<float3> position_in : register(t0, space0);
StructuredBuffer<float3> normal_in : register(t1, space0);
StructuredBuffer<uint4> bone_index : register(t2, space0);
StructuredBuffer<float3> weight : register(t3, space0);

RWStructuredBuffer<float3> position_out : register(u0, space0);
RWStructuredBuffer<float3> normal_out : register(u1, space0);

[numthreads(256, 1, 1)]
void cs_main(int3 dtid : SV_DispatchThreadID)
{
    float weights[4] = { 0.0, 0.0, 0.0, 0.0 };
    weights[0] = weight[dtid.x].x;
    weights[1] = weight[dtid.x].y;
    weights[2] = weight[dtid.x].z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float4x4 m = float4x4(
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = 0; i < 4; ++i)
        m += weights[i] * offset[bone_index[dtid.x][i]];

    position_out[dtid.x] = mul(float4(position_in[dtid.x], 1.0f), m).xyz;
    normal_out[dtid.x] = mul(float4(normal_in[dtid.x], 0.0f), m).xyz;
}