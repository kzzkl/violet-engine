[[vk::binding(0, 0)]]
cbuffer ui : register(b0, space0)
{
    float4x4 ui_mvp;
};

[[vk::binding(1, 0)]]
Texture2D ui_texture : register(t0, space0);

SamplerState sampler_ui : register(s7);

struct vs_in
{
    float2 position : POSITION;
    float2 uv : UV;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 uv : UV;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;
    result.position = mul(float4(vin.position, 0.0f, 1.0f), ui_mvp);
    result.uv = vin.uv;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    // return float4(1.0f, 1.0f, 1.0f, 1.0f);
    float4 color = ui_texture.Sample(sampler_ui, pin.uv);
    return float4(color.x, 0.0f, 0.0f, 1.0f);
}