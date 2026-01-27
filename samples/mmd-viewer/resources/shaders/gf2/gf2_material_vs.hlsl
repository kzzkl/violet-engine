#include "gf2/gf2_material.hlsli"

struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id, scene);
    vertex vertex = mesh.fetch_vertex(vertex_id, camera.matrix_vp);

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.position_ws = vertex.position_ws;
    output.normal_ws = vertex.normal_ws;
    output.tangent_ws = vertex.tangent_ws;
    output.bitangent_ws = vertex.bitangent_ws;
    output.texcoord = vertex.texcoord;
    output.texcoord2 = mesh.fetch_custom_attribute<float4>(vertex_id, 1);
    output.material_address = mesh.get_material_address();

    return output;
}