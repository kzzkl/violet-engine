struct violet_ui_mvp
{
    float4x4 mvp;
};
ConstantBuffer<violet_ui_mvp> mvp : register(b0, space0);

struct violet_ui_material
{
    uint type;
};
ConstantBuffer<violet_ui_material> material : register(b0, space1);
Texture2D<float4> material_texture : register(t1, space1);
SamplerState material_texture_sampler : register(s1, space1);

struct vs_in
{
    float2 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
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

    float4 position = float4(input.position, 0.0, 1.0);

    vs_out output;
    output.position = mul(mvp.mvp, position);
    output.uv = input.uv;
    output.color = input.color;

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    if (material.type == 2)
    {
        float4 texture_color = material_texture.Sample(material_texture_sampler, input.uv);
        return input.color * texture_color;
    }
    else if (material.type == 1)
    {
        float alpha = material_texture.Sample(material_texture_sampler, input.uv).r;
        return float4(input.color.rgb, alpha);
    }
    else
    {
        return input.color;
    }
}