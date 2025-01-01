struct draw_constant
{
    float4x4 mvp;
    uint4 textures[16];
};
ConstantBuffer<draw_constant> constant : register(b0, space1);

struct vs_input
{
    float2 position : POSITION;
    float2 uv : UV;
    uint color : COLOR;
};

struct vs_output
{
    float4 position : SV_POSITION;
    float2 uv : UV;
    float4 color : COLOR;
    uint texture : TEXTURE;
};

vs_output vs_main(vs_input input, uint instance_index : SV_InstanceID)
{
    vs_output output;
    output.position = mul(constant.mvp, float4(input.position, 0.0f, 1.0f));
    output.uv = input.uv;
    output.color.r = (input.color & 0xFF) / 255.0f;
    output.color.g = ((input.color >> 8) & 0xFF) / 255.0f;
    output.color.b = ((input.color >> 16) & 0xFF) / 255.0f;
    output.color.a = ((input.color >> 24) & 0xFF) / 255.0f;
    output.texture = constant.textures[instance_index >> 2][instance_index & 3];

    return output;
}

float4 fs_main(vs_output input) : SV_TARGET
{
    Texture2D texture = ResourceDescriptorHeap[input.texture];
    SamplerState linear_repeat_sampler = SamplerDescriptorHeap[1];

    float4 color = input.color * texture.Sample(linear_repeat_sampler, input.uv);
    color.rgb = pow(color.rgb, 2.2);
    return color;
}