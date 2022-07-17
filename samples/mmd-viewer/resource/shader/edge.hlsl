cbuffer ash_object : register(b0, space0)
{
    float4x4 transform_m;
};

cbuffer mmd_material : register(b0, space1)
{
    float4 diffuse;
    float3 specular;
    float specular_strength;
    float4 edge_color;
    float3 ambient;
    float edge_size;
    uint toon_mode;
    uint spa_mode;
};

cbuffer ash_camera : register(b0, space2)
{
    float3 camera_position;
    float3 camera_direction;

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
    float4 position = mul(mul(float4(vin.position, 1.0f), transform_m), transform_vp);
    float4 normal = mul(mul(float4(vin.normal, 0.0f), transform_m), transform_v);
    float2 screen_normal = normalize(normal.xy);

    position.xy += screen_normal * 0.003f * edge_size * position.w;

    vs_out result;
    result.position = position;
    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return edge_color;
}