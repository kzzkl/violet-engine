struct vs_in
{
    [[vk::location(0)]] float3 position: POSITION;
    [[vk::location(1)]] float3 color: COLOR;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

struct UBO
{
	float4x4 mvp;
};

cbuffer ubo : register(b0, space0) { UBO ubo; }

vs_out vs_main(vs_in input)
{
    vs_out output;

    output.position = mul(ubo.mvp, float4(input.position, 1.0));
    output.color = input.color;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(input.color, 1.0);
}