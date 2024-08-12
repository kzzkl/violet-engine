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
    const float2 ndc[4] = {
        float2(-1.0, 1.0),
        float2(1.0, 1.0),
        float2(1.0, -1.0),
        float2(-1.0, -1.0),
    };

    const float2 uv[4] = {
        float2(0.0, 0.0),
        float2(1.0, 0.0),
        float2(1.0, 1.0),
        float2(0.0, 1.0),
    };
    
    const uint indices[6] = {0, 2, 1, 0, 3, 2};

    uint index = indices[input.vertex_id];

    vs_out output;
    output.position = float4(ndc[index], 0.0, 1.0);
    output.uv = uv[index];
    return output;
}