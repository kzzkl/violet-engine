struct violet_ui_offset
{
    float4 offset[1024];
};
ConstantBuffer<violet_ui_offset> offset : register(b0, space0);

struct violet_ui_mvp
{
    float4x4 mvp;
};
ConstantBuffer<violet_ui_mvp> mvp : register(b0, space1);

struct vs_in
{
    float2 position : POSITION;
    // float2 uv : UV;
    float4 color : COLOR;
    // uint offset_index : OFFSET_INDEX;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 uv : UV;
    float4 color : COLOR;
};

vs_out vs_main(vs_in input)
{
    // float4 position = offset.offset[input.offset_index];
    // position.xy += input.position;

    vs_out output;
    // output.position = mul(position, mvp.mvp);
    output.position = float4(input.position, 0.0, 1.0);
    // output.uv = input.uv;
    output.color = input.color;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return float4(1.0, 0.0, 0.0, 1.0);
    return input.color;
}