struct vs_output
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

vs_output vs_main(uint vertex_id : SV_VertexID)
{
    const float2 ndc[3] = {
        float2(-1.0, 1.0),
        float2(-1.0, -3.0),
        float2(3.0, 1.0),
    };

    const float2 texcoord[3] = {
        float2(0.0, 0.0),
        float2(0.0, 2.0),
        float2(2.0, 0.0),
    };

    vs_output output;
    output.position = float4(ndc[vertex_id], 0.0, 1.0);
    output.texcoord = texcoord[vertex_id];
    return output;
}