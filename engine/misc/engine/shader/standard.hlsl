[[vk::binding(0, 0)]]
cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
    float4x4 transform_mv;
    float4x4 transform_mvp;
};

[[vk::binding(0, 1)]]
cbuffer mmd_material : register(b0, space1)
{
    float3 color;
};

[[vk::binding(0, 2)]]
cbuffer ash_pass : register(b0, space2)
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
    float3 normal : NORMAL;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    result.position = mul(float4(vin.position, 1.0f), transform_mvp);
    result.normal = mul(float4(vin.normal, 0.0f), transform_m).xyz;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    float3 light_dir = normalize(float3(-1.0f, 0.8f, -0.6f));
    float3 normal = normalize(pin.normal);
    float c = dot(normal, light_dir);

    return float4(color * (0.5f + c * 0.5f), 1.0f);
}