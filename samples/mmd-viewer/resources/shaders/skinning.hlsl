struct mmd_bone
{
    vec4fx3 offset;
    vec4f quaternion;
};

struct mmd_skeleton
{
    mmd_bone bones[1024];
};

StructuredBuffer<float> position_in : register(t0, space0);
StructuredBuffer<float> normal_in : register(t1, space0);
RWStructuredBuffer<float> position_out : register(u2, space0);
RWStructuredBuffer<float> normal_out : register(u3, space0);
ConstantBuffer<mmd_skeleton> skeleton: register(b4, space0);

struct bdef_data
{
    uint4 index;
    vec4f weight;
};
StructuredBuffer<bdef_data> bdef : register(t5, space0);

struct sdef_data
{
    uint2 index;
    vec2f weight;
    vec3f center;
    float _padding_0;
    vec3f r0;
    float _padding_1;
    vec3f r1;
    float _padding_2;
};
StructuredBuffer<sdef_data> sdef : register(t6, space0);

StructuredBuffer<uint2> skin : register(t7, space0);
StructuredBuffer<float> morph : register(t8, space0);

vec4f quaternion_slerp(vec4f a, vec4f b, float t)
{
    float cos_omega = dot(a, b);

    vec4f c = b;
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

    return vec4f(
        a[0] * k0 + c[0] * k1,
        a[1] * k0 + c[1] * k1,
        a[2] * k0 + c[2] * k1,
        a[3] * k0 + c[3] * k1);
}

vec3fx3 quaternion_to_matrix(vec4f q)
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

    return vec3fx3(
        1.0f - yyd - zzd, xyd + zwd,        xzd - ywd,
        xyd - zwd,        1.0f - xxd - zzd, yzd + xwd,
        xzd + ywd,        yzd - xwd,        1.0f - xxd - yyd);
}

[numthreads(256, 1, 1)]
void cs_main(int3 dtid : SV_DispatchThreadID)
{
    int index = dtid.x * 3;
    vec3f position = vec3f(
        position_in[index + 0] + morph[index + 0],
        position_in[index + 1] + morph[index + 1],
        position_in[index + 2] + morph[index + 2]);
    vec3f normal = vec3f(normal_in[index + 0], normal_in[index + 1], normal_in[index + 2]);

    uint skin_type = skin[dtid.x].x;
    uint skin_index = skin[dtid.x].y;

    if (skin_type == 0)
    {
        vec4fx3 m = vec4fx3(
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i < 4; ++i)
            m += bdef[skin_index].weight[i] * skeleton.bones[bdef[skin_index].index[i]].offset;

        position = mul(vec4f(position, 1.0f), m).xyz;
        normal = mul(normal, (vec3fx3)m);
    }
    else
    {
        vec4f q0 = skeleton.bones[sdef[skin_index].index[0]].quaternion;
        vec4f q1 = skeleton.bones[sdef[skin_index].index[1]].quaternion;
        vec4fx3 m0 = skeleton.bones[sdef[skin_index].index[0]].offset;
        vec4fx3 m1 = skeleton.bones[sdef[skin_index].index[1]].offset;

        float w0 = sdef[skin_index].weight[0];
        float w1 = sdef[skin_index].weight[1];
        vec3f center = sdef[skin_index].center;

        vec3fx3 rotate_m = quaternion_to_matrix(quaternion_slerp(q0, q1, w1));

        position = mul(position - center, rotate_m);
        position += (mul(vec4f(sdef[skin_index].r0, 1.0f), m0) * w0).xyz;
        position += (mul(vec4f(sdef[skin_index].r1, 1.0f), m1) * w1).xyz;

        normal = mul(normal, rotate_m);
    }

    position_out[index + 0] = position.x;
    position_out[index + 1] = position.y;
    position_out[index + 2] = position.z;

    normal_out[index + 0] = normal.x;
    normal_out[index + 1] = normal.y;
    normal_out[index + 2] = normal.z;
}