cbuffer ash_object : register(b0)
{
    float4x4 transform_m;
    float4x4 transform_mv;
    float4x4 transform_mvp;
};

cbuffer mmd_material : register(b1)
{
    float4 color;
    float4 color2;
    float4 color3;
};

cbuffer ash_pass : register(b2)
{
    float4 camera_position;
    float4 camera_direction;

    float4x4 transform_v;
    float4x4 transform_p;
    float4x4 transform_vp;
};

Texture2D tex : register(t0);

SamplerState sampler_wrap : register(s0);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : UV;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    result.position = mul(float4(vin.position, 1.0f), transform_mvp);
    result.color = float4(vin.normal, 1.0f) * color;
    result.uv = vin.uv;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return tex.Sample(sampler_wrap, pin. uv);
    // return pin.color;
}