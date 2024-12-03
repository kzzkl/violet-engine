struct vs_in
{
    uint vertex_id : SV_VertexID;
};

struct vs_out
{
    float4 position : SV_POSITION;
    
    float3 right : NORMAL0;
    float3 left : NORMAL1;
    float3 top : NORMAL2;
    float3 bottom : NORMAL3;
    float3 front : NORMAL4;
    float3 back : NORMAL5;
};

vs_out vs_main(vs_in input)
{
    const float2 ndc[4] = {
        float2(-1.0, 1.0),
        float2(1.0, 1.0),
        float2(1.0, -1.0),
        float2(-1.0, -1.0),
    };

    const float3 vertices[24] = {
        float3(1.0, 1.0, 1.0),   float3(1.0, 1.0, -1.0),  float3(1.0, -1.0, -1.0),  float3(1.0, -1.0, 1.0),     // right
        float3(-1.0, 1.0, -1.0), float3(-1.0, 1.0, 1.0),  float3(-1.0, -1.0, 1.0),  float3(-1.0, -1.0, -1.0),   // left
        float3(-1.0, 1.0, -1.0), float3(1.0, 1.0, -1.0),  float3(1.0, 1.0, 1.0),    float3(-1.0, 1.0, 1.0),     // top
        float3(-1.0, -1.0, 1.0), float3(1.0, -1.0, 1.0),  float3(1.0, -1.0, -1.0),  float3(-1.0, -1.0, -1.0),   // bottom
        float3(-1.0, 1.0, 1.0),  float3(1.0, 1.0, 1.0),   float3(1.0, -1.0, 1.0),   float3(-1.0, -1.0, 1.0),    // front
        float3(1.0, 1.0, -1.0),  float3(-1.0, 1.0, -1.0), float3(-1.0, -1.0, -1.0), float3(1.0, -1.0, -1.0)     // back
    };

    const int indexes[36] = {
        0,  2,  1,  0,  3,  2,  // right
        4,  6,  5,  4,  7,  6,  // left
        8,  10, 9,  8,  11, 10, // up
        12, 14, 13, 12, 15, 14, // down
        16, 18, 17, 16, 19, 18, // forward
        20, 22, 21, 20, 23, 22  // back
    };

    vs_out output;
    output.position = float4(ndc[indexes[input.vertex_id]], 0.0, 1.0);

    output.right = normalize(vertices[indexes[input.vertex_id + 0]]);
    output.left = normalize(vertices[indexes[input.vertex_id + 6]]);
    output.top = normalize(vertices[indexes[input.vertex_id + 12]]);
    output.bottom = normalize(vertices[indexes[input.vertex_id + 18]]);
    output.front = normalize(vertices[indexes[input.vertex_id + 24]]);
    output.back = normalize(vertices[indexes[input.vertex_id + 30]]);

    return output;
}