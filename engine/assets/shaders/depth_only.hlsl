#include "mesh.hlsli"

struct vs_output
{
    float4 position_cs : SV_POSITION;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    mesh mesh = mesh::create(instance_id, vertex_id);
    vertex vertex = mesh.fetch_vertex();

    vs_output output;
    output.position_cs = vertex.position_cs;

    return output;
}