cbuffer ui : register(b0)
{
    float4x4 ui_mvp;
};

SamplerState sampler_ui : register(s7);
Texture2D ui_texture : register(t0);

struct vs_in
{
    float2 position : POSITION;
    float2 uv : UV;
    float4 color : COL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 uv : UV;
    float4 color : COLOR;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;
    result.position = mul(ui_mvp, float4(vin.position, 0.0f, 1.0f));
    result.uv = vin.uv;
    result.color = vin.color;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return pin.color * ui_texture.Sample(sampler_ui, pin.uv);
}