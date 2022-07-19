cbuffer mmd_skeleton : register(b0, space0)
{
    float4x3 bone_offset[512];
    float4 bone_quaternion[512];
};

StructuredBuffer<float3> position_in : register(t0, space0);
StructuredBuffer<float3> normal_in : register(t1, space0);
StructuredBuffer<float2> uv_in : register(t2, space0);

StructuredBuffer<uint2> skin : register(t3, space0);

struct bdef_data
{
    uint4 index;
    float4 weight;
};
StructuredBuffer<bdef_data> bdef_bone : register(t4, space0);

struct sdef_data
{
    uint2 index;
    float2 weight;
    float3 center;
    float _padding_0;
    float3 r0;
    float _padding_1;
    float3 r1;
    float _padding_2;
};
StructuredBuffer<sdef_data> sdef_bone : register(t5, space0);

StructuredBuffer<float3> vertex_morph : register(t6, space0);
StructuredBuffer<float2> uv_morph : register(t7, space0);

RWStructuredBuffer<float3> position_out : register(u0, space0);
RWStructuredBuffer<float3> normal_out : register(u1, space0);
RWStructuredBuffer<float2> uv_out : register(u2, space0);

float4 quaternion_slerp(float4 a, float4 b, float t)
{
    float cos_omega = dot(a, b);

    float4 c = b;
    if (cos_omega < 0.0f)
    {
        c = -b;
        cos_omega = -cos_omega;
    }

    float k0, k1;
    if (cos_omega > 0.9999f)
    {
        k0 = 1.0f - t;
        k1 = t;
    }
    else
    {
        float sin_omega = sqrt(1.0f - cos_omega * cos_omega);
        float omega = atan2(sin_omega, cos_omega);
        float div = 1.0f / sin_omega;
        k0 = sin((1.0f - t) * omega) * div;
        k1 = sin(t * omega) * div;
    }

    return float4(
        a[0] * k0 + c[0] * k1,
        a[1] * k0 + c[1] * k1,
        a[2] * k0 + c[2] * k1,
        a[3] * k0 + c[3] * k1);
}

float3x3 quaternion_to_matrix(float4 q)
{
    float xxd = 2.0f * q[0] * q[0];
    float xyd = 2.0f * q[0] * q[1];
    float xzd = 2.0f * q[0] * q[2];
    float xwd = 2.0f * q[0] * q[3];
    float yyd = 2.0f * q[1] * q[1];
    float yzd = 2.0f * q[1] * q[2];
    float ywd = 2.0f * q[1] * q[3];
    float zzd = 2.0f * q[2] * q[2];
    float zwd = 2.0f * q[2] * q[3];
    float wwd = 2.0f * q[3] * q[3];

    return float3x3(
        1.0f - yyd - zzd, xyd + zwd,        xzd - ywd,
        xyd - zwd,        1.0f - xxd - zzd, yzd + xwd,
        xzd + ywd,        yzd - xwd,        1.0f - xxd - yyd);
}

[numthreads(256, 1, 1)]
void cs_main(int3 dtid : SV_DispatchThreadID)
{
    uint skin_type = skin[dtid.x].x;
    uint skin_index = skin[dtid.x].y;
    if (skin_type == 0)
    {
        float4x3 m = float4x3(
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 4; ++i)
            m += bdef_bone[skin_index].weight[i] * bone_offset[bdef_bone[skin_index].index[i]];

        float3 position = position_in[dtid.x] + vertex_morph[dtid.x];
        position_out[dtid.x] = mul(float4(position, 1.0f), m).xyz;
        normal_out[dtid.x] = mul(normal_in[dtid.x], (float3x3)m);
    }
    else if (skin_type == 1)
    {
        float4 q0 = bone_quaternion[sdef_bone[skin_index].index[0]];
        float4 q1 = bone_quaternion[sdef_bone[skin_index].index[1]];
        float4x3 m0 = bone_offset[sdef_bone[skin_index].index[0]];
        float4x3 m1 = bone_offset[sdef_bone[skin_index].index[1]];

        float w0 = sdef_bone[skin_index].weight[0];
        float w1 = sdef_bone[skin_index].weight[1];
        float3 center = sdef_bone[skin_index].center;

        float3x3 rotate_m = quaternion_to_matrix(quaternion_slerp(q0, q1, w1));

        float3 position = position_in[dtid.x] + vertex_morph[dtid.x];
        float3 p = position;
        position = mul(position - center, rotate_m);
        position += (mul(float4(sdef_bone[skin_index].r0, 1.0f), m0) * w0).xyz;
        position += (mul(float4(sdef_bone[skin_index].r1, 1.0f), m1) * w1).xyz;

        position_out[dtid.x] = position;
        normal_out[dtid.x] = mul(normal_in[dtid.x], (float3x3)rotate_m);
    }

    uv_out[dtid.x] = uv_in[dtid.x] + uv_morph[dtid.x];
}