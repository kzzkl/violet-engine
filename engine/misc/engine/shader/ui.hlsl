cbuffer ui_material : register(b0, space0)
{
    uint type;
};
Texture2D ui_texture : register(t0, space0);

cbuffer ui_mvp : register(b0, space1)
{
    float4x4 mvp;
}

SamplerState sampler_ui : register(s7);

struct vs_in
{
    float2 position : POSITION;
    float2 uv : UV;
    float4 color : COLOR;
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
    result.position = mul(float4(vin.position, 0.0f, 1.0f), mvp);
    result.uv = vin.uv;
    result.color = vin.color;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    if (type == 2) // text
    {
        float4 color = ui_texture.Sample(sampler_ui, pin.uv);
        return float4(pin.color.rgb, color.r * pin.color.a);
    }
    else if (type == 3) // image
    {
        return ui_texture.Sample(sampler_ui, pin.uv);
    }
    else
    {
        return pin.color;
    }
}