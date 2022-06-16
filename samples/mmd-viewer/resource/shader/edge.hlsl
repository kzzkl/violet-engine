cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
    float4x4 transform_mv;
    float4x4 transform_mvp;
};

cbuffer ash_pass : register(b0, space1)
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
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in vin)
{
    float4 position = mul(float4(vin.position, 1.0f), transform_mv);
    float4 normal = mul(float4(vin.normal, 0.0f), transform_mv);
    normal = normalize(normal);
    normal.z -= 0.4f;
    position += normal * 0.01f;

    position = mul(position, transform_p);

    vs_out result;
    result.position = position;
    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}