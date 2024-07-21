struct vs_out
{
    float4 position: SV_POSITION;
    float3 color: COLOR;
};

vs_out vs_main(uint index: SV_VertexID)
{
    float2 positions[3] = {
        float2(0.0, -0.5),
        float2(0.5, 0.5),
        float2(-0.5, 0.5)
    };

    float3 colors[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)
    };

    vs_out output;

    output.position = float4(positions[index], 0.0, 1.0);
    output.color = colors[index];

    return output;
}

float4 fs_main(vs_out input) : SV_TARGET
{
    return float4(input.color, 1.0);
}