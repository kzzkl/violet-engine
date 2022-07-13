cbuffer ui_material : register(b0, space0)
{
    uint type;
};
Texture2D ui_texture : register(t0, space0);

cbuffer ui_offset : register(b0, space1)
{
    float4 offset[1024];
}

cbuffer ui_mvp : register(b0, space2)
{
    float4x4 mvp;
}

SamplerState image_sampler : register(s0);
SamplerState text_sampler : register(s7);

struct vs_in
{
    float2 position : POSITION;
    float2 uv : UV;
    float4 color : COLOR;
    uint offset_index : OFFSET_INDEX;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 uv : UV;
    float4 color : COLOR;
};

vs_out vs_main(vs_in vin)
{
    float4 position = offset[vin.offset_index];
    position.xy += vin.position;

    vs_out result;
    result.position = mul(position, mvp);
    result.uv = vin.uv;
    result.color = vin.color;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    if (type == 0)
    {
        return pin.color;
    }
    else if (type == 1) // text
    {
        float4 color = ui_texture.Sample(text_sampler, pin.uv);

        // Gamma correct
        color = pow(color, 1.0f / 2.2f);
        return float4(pin.color.rgb, color.r * pin.color.a);
    }
    else if (type == 2) // image 
    {
        float4 color = ui_texture.Sample(image_sampler, pin.uv);
        return float4(color.rgb, 1.0f);
    }
    else
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}