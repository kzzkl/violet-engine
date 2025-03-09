#include "gf2/gf2_material.hlsli"

vs_output vs_main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    mesh mesh = mesh::create(instance_id, vertex_id);
    vertex vertex = mesh.fetch_vertex();

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.position_ws = vertex.position_ws;
    output.normal_ws = vertex.normal_ws;
    output.tangent_ws = vertex.tangent_ws;
    output.bitangent_ws = vertex.bitangent_ws;
    output.texcoord = vertex.texcoord;
    output.texcoord2 = mesh.fetch_custom_attribute<float4>(1);
    output.material_address = mesh.get_material_address();

    return output;
}