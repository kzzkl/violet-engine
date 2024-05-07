struct vs_in
{
    uint vertex_id : SV_VertexID;
};

struct vs_out
{
    float4 position : SV_POSITION;
};

vs_out vs_main(vs_in input)
{
    const float2 vertices[6] = {
        float2(1.0, -1.0),
        float2(1.0, 1.0),
        float2(-1.0, -1.0),
        float2(-1.0, -1.0),
        float2(1.0, 1.0),
        float2(-1.0, 1.0)};

    vs_out output;
    output.position = float4(vertices[input.vertex_id], 0.0, 1.0);

    return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
    // float4 color = float4(input.color, 1.0);
    // color *= texture.Sample(texture_sampler, input.uv);
    // return color;
    return  float4(1.0, 0.0, 0.0, 1.0);
}