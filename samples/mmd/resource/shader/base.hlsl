cbuffer object : register(b0)
{
    float4x4 model_view_proj;
};

cbuffer material : register(b1)
{
    float4 color;
    float4 color2;
}

struct vs_in
{
    float3 position : POSITION;
    float4 color : COLOR;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

vs_out vs_main(vs_in vin)
{
    vs_out result;

    result.position = mul(float4(vin.position, 1.0f), model_view_proj);
    result.color = color;

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return pin.color;
}