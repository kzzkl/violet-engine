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

struct vs_input
{
    float2 position : POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    // uint offset_index : OFFSET_INDEX;
};

struct vs_output
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
    float4 color : COLOR;
};

vs_output vs_main(vs_input input)
{
    // float4 position = offset.offset[input.offset_index];
    // position.xy += input.position;

    float4 position = float4(input.position, 0.0, 1.0);

    vs_output output;
    output.position = mul(mvp.mvp, position);
    output.texcoord = input.texcoord;
    output.color = input.color;

    return output;
}

float4 fs_main(vs_output input) : SV_TARGET
{
    if (material.type == 2)
    {
        float4 texture_color = material_texture.Sample(material_texture_sampler, input.texcoord);
        return input.color * texture_color;
    }
    else if (material.type == 1)
    {
        float alpha = material_texture.Sample(material_texture_sampler, input.texcoord).r;
        return float4(input.color.rgb, alpha);
    }
    else
    {
        return input.color;
    }
}