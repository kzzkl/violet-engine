cbuffer ash_pass : register(b0)
{
    float4 camera_position;
    float4 camera_direction;

    float4x4 camera_view;
    float4x4 camera_projection;
    float4x4 camera_view_projection;
};

cbuffer material : register(b1)
{
    float4 color;
    float4 color2;
}

struct vs_in
{
    float3 position : POSITION;
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
    result.color = float4(0.5f, 0.5f, 0.5f, 1.0f);

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return pin.color;
}