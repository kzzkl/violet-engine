Texture2D source : register(t0, space0);
SamplerState source_sampler : register(s0, space0);

struct vs_out
{
    float4 position: SV_POSITION;
    float2 uv: UV;
};

vs_out vs_main(uint index: SV_VertexID)
{
    const float2 ndc[4] = {
        float2(1.0, 1.0),
        float2(-1.0, 1.0),
        float2(-1.0, -1.0),
        float2(1.0, -1.0),
    };

    const float2 uv[4] = {
        float2(1.0, 1.0),
        float2(-1.0, 1.0),
        float2(-1.0, -1.0),
        float2(1.0, -1.0),
    };

    const int indices[6] = {
        0, 1, 2, 2, 3, 0
    };

    vs_out output;
    output.position = float4(ndc[indices[index]], 0.0, 1.0);
    output.uv = uv[indices[index]];

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    return source.Sample(source_sampler, input.uv);
}