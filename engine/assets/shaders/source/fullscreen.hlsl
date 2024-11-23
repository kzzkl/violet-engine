struct vs_in
{
    uint vertex_id : SV_VertexID;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

vs_out vs_main(vs_in input)
{
    const float2 ndc[3] = {
        float2(-1.0, 1.0),
        float2(-1.0, -3.0),
        float2(3.0, 1.0),
    };

    const float2 uv[3] = {
        float2(0.0, 0.0),
        float2(0.0, 2.0),
        float2(2.0, 0.0),
    };

    vs_out output;
    output.position = float4(ndc[input.vertex_id], 0.0, 1.0);
    output.uv = uv[input.vertex_id];
    return output;
}