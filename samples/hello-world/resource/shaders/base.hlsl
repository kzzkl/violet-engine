struct vs_out
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

vs_out vs_main(uint vertex_id
               : SV_VertexID)
{
    float2 position[3] = {
        float2(0.0, -0.5),
        float2(0.5, 0.5),
        float2(-0.5, 0.5)};

    float3 color[3] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0)};

    vs_out result;

    result.position = float4(position[vertex_id], 0.0, 1.0);
    result.color = color[vertex_id];

    return result;
}

float4 ps_main(vs_out pin) : SV_TARGET
{
    return float4(pin.color, 1.0);
}