#include "mesh.hlsli"
#include "visibility/visibility_utils.hlsli"

struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

struct vs_output
{
    float4 position_cs : SV_POSITION;
    uint instance_id : INSTANCE_ID;
    uint primitive_offset : PRIMITIVE_OFFSET;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;
    uint cluster_id = draw_infos[draw_id].cluster_id;

    mesh mesh = mesh::create(instance_id);
    vertex vertex = mesh.fetch_vertex(vertex_id);

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.instance_id = instance_id;

    if (cluster_id != 0xFFFFFFFF)
    {
        StructuredBuffer<cluster_data> clusters = ResourceDescriptorHeap[scene.cluster_buffer];
        cluster_data cluster = clusters[cluster_id];

        output.primitive_offset = cluster.index_offset / 3;
    }
    else
    {
        output.primitive_offset = 0;
    }

    return output;
}

uint2 fs_main(vs_output input, uint primitive_id : SV_PrimitiveID) : SV_Target0
{
    return pack_visibility(input.instance_id, input.primitive_offset + primitive_id);
}