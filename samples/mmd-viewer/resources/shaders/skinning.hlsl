struct mmd_bone
{
    float4x3 offset;
    float4 quaternion;
};

struct mmd_skeleton
{
    mmd_bone bones[1024];
};

ConstantBuffer<mmd_skeleton> skeleton: register(b0, space0);

StructuredBuffer<float> position_in : register(t0, space1);
StructuredBuffer<float> normal_in : register(t1, space1);
StructuredBuffer<float2> uv_in : register(t2, space1);
RWStructuredBuffer<float> position_out : register(u3, space1);
RWStructuredBuffer<float> normal_out : register(u4, space1);
RWStructuredBuffer<float2> uv_out : register(u5, space1);

struct bdef_data
{
    uint4 index;
    float4 weight;
};
StructuredBuffer<bdef_data> bdef : register(t6, space1);

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
StructuredBuffer<sdef_data> sdef : register(t7, space1);

StructuredBuffer<uint2> skin : register(t8, space1);
StructuredBuffer<float> vertex_morph : register(t9, space1);

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
    int index = dtid.x * 3;
    float3 position = float3(
        position_in[index + 0] + vertex_morph[index + 0],
        position_in[index + 1] + vertex_morph[index + 1],
        position_in[index + 2] + vertex_morph[index + 2]);
    float3 normal = float3(normal_in[index + 0], normal_in[index + 1], normal_in[index + 2]);

    uint skin_type = skin[dtid.x].x;
    uint skin_index = skin[dtid.x].y;

    if (skin_type == 0)
    {
        float4x3 m = float4x3(
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 4; ++i)
            m += bdef[skin_index].weight[i] * skeleton.bones[bdef[skin_index].index[i]].offset;

        position = mul(float4(position, 1.0f), m).xyz;
        normal = mul(normal, (float3x3)m);
    }
    else
    {
        float4 q0 = skeleton.bones[sdef[skin_index].index[0]].quaternion;
        float4 q1 = skeleton.bones[sdef[skin_index].index[1]].quaternion;
        float4x3 m0 = skeleton.bones[sdef[skin_index].index[0]].offset;
        float4x3 m1 = skeleton.bones[sdef[skin_index].index[1]].offset;

        float w0 = sdef[skin_index].weight[0];
        float w1 = sdef[skin_index].weight[1];
        float3 center = sdef[skin_index].center;

        float3x3 rotate_m = quaternion_to_matrix(quaternion_slerp(q0, q1, w1));

        position = mul(position - center, rotate_m);
        position += (mul(float4(sdef[skin_index].r0, 1.0f), m0) * w0).xyz;
        position += (mul(float4(sdef[skin_index].r1, 1.0f), m1) * w1).xyz;

        normal = mul(normal, rotate_m);
    }

    position_out[index + 0] = position.x;
    position_out[index + 1] = position.y;
    position_out[index + 2] = position.z;

    normal_out[index + 0] = normal.x;
    normal_out[index + 1] = normal.y;
    normal_out[index + 2] = normal.z;

    uv_out[dtid.x] = uv_in[dtid.x];
}