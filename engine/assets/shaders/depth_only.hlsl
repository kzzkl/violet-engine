#include "mesh.hlsli"

struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

struct vs_output
{
    float4 position_cs : SV_POSITION;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id);
    vertex vertex = mesh.fetch_vertex(vertex_id);

    vs_output output;
    output.position_cs = vertex.position_cs;

    return output;
}