#include "common.hlsli"

struct constant_data
{
    uint position_input_address;
    uint normal_input_address;
    uint tangent_input_address;
    uint position_output_address;
    uint normal_output_address;
    uint tangent_output_address;
    uint vertex_buffer;
    uint skeleton;
    uint skin;
    uint bdef;
    uint sdef;
    uint morph;
};
PushConstant(constant_data, constant);

struct bdef_data
{
    uint4 index;
    float4 weight;
};

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

float4 matrix_to_quaternion(float4x4 m)
{
    float4 q;
    float t;

    if (m[2][2] < 0.0f) // x^2 + y ^2 > z^2 + w^2
    {
        if (m[0][0] > m[1][1]) // x > y
        {
            t = 1.0f + m[0][0] - m[1][1] - m[2][2];
            q = float4(t, m[0][1] + m[1][0], m[2][0] + m[0][2], m[1][2] - m[2][1]);
        }
        else
        {
            t = 1.0f - m[0][0] + m[1][1] - m[2][2];
            q = float4(m[0][1] + m[1][0], t, m[1][2] + m[2][1], m[2][0] - m[0][2]);
        }
    }
    else
    {
        if (m[0][0] < -m[1][1]) // z > w
        {
            t = 1.0f - m[0][0] - m[1][1] + m[2][2];
            q = float4(m[2][0] + m[0][2], m[1][2] + m[2][1], t, m[0][1] - m[1][0]);
        }
        else
        {
            t = 1.0f + m[0][0] + m[1][1] + m[2][2];
            q = float4(m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0], t);
        }
    }

    return q * 0.5f / sqrt(t);
}

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWByteAddressBuffer vertex_buffer = ResourceDescriptorHeap[constant.vertex_buffer];

    StructuredBuffer<uint2> skin = ResourceDescriptorHeap[constant.skin];
    StructuredBuffer<float4x4> skeleton = ResourceDescriptorHeap[constant.skeleton];

    uint vertex_index = dtid.x;

    float3 position = vertex_buffer.Load<float3>(constant.position_input_address + vertex_index * sizeof(float3));
    float3 normal = vertex_buffer.Load<float3>(constant.normal_input_address + vertex_index * sizeof(float3));
    float4 tangent = vertex_buffer.Load<float4>(constant.tangent_input_address + vertex_index * sizeof(float4));

    if (constant.morph != 0)
    {
        position += get_morph_position(constant.morph, vertex_index);
    }

    uint skin_type = skin[dtid.x].x;
    uint skin_index = skin[dtid.x].y;

    if (skin_type == 0 && constant.bdef != 0)
    {
        StructuredBuffer<bdef_data> bdef = ResourceDescriptorHeap[constant.bdef];

        float4x4 m = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            m += bdef[skin_index].weight[i] * skeleton[bdef[skin_index].index[i]];
        }

        position = mul(m, float4(position, 1.0)).xyz;
        normal = mul((float3x3)m, normal);
        tangent.xyz = mul((float3x3)m, tangent.xyz);
    }
    else if (constant.sdef != 0)
    {
        StructuredBuffer<sdef_data> sdef = ResourceDescriptorHeap[constant.sdef];

        float4x4 m0 = skeleton[sdef[skin_index].index[0]];
        float4x4 m1 = skeleton[sdef[skin_index].index[1]];
        float4 q0 = matrix_to_quaternion(m0);
        float4 q1 = matrix_to_quaternion(m1);

        float w0 = sdef[skin_index].weight[0];
        float w1 = sdef[skin_index].weight[1];
        float3 center = sdef[skin_index].center;

        float3x3 rotate_m = quaternion_to_matrix(quaternion_slerp(q0, q1, w1));

        position = mul(rotate_m, position - center);
        position += (mul(m0, float4(sdef[skin_index].r0, 1.0f)) * w0).xyz;
        position += (mul(m1, float4(sdef[skin_index].r1, 1.0f)) * w1).xyz;

        normal = mul(rotate_m, normal);
        tangent.xyz = mul((float3x3)rotate_m, tangent.xyz);
    }

    vertex_buffer.Store<float3>(constant.position_output_address + vertex_index * sizeof(float3), position);
    vertex_buffer.Store<float3>(constant.normal_output_address + vertex_index * sizeof(float3), normal);
    vertex_buffer.Store<float4>(constant.tangent_output_address + vertex_index * sizeof(float4), tangent);
}