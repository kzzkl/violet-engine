cbuffer ash_object : register(b0)
{
    float4x4 to_world;
};

cbuffer mmd_material : register(b1)
{
    float4 color;
    float4 color2;
};

cbuffer ash_pass : register(b2)
{
    float4 camera_position;
    float4 camera_direction;

    float4x4 camera_view;
    float4x4 camera_projection;
    float4x4 camera_view_projection;
};

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    result.position = mul(float4(vin.position, 1.0f), camera_view_projection);
    result.color = float4(vin.normal, 1.0f);

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return pin.color;
}