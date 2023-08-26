struct vs_in
{
    [[vk::location(0)]] float2 position: POSITION;
    [[vk::location(1)]] float3 color: COLOR;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

vs_out vs_main(vs_in input)
{
    vs_out output;

    output.position = float4(input.position, 0.0, 1.0);
    output.color = input.color;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(input.color, 1.0);
}