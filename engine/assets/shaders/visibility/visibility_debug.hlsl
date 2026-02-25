#include "cluster.hlsli"
#include "mesh.hlsli"
#include "utils.hlsli"

struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

#ifndef DEBUG_MODE_CLUSTER
#define DEBUG_MODE_CLUSTER 0
#endif

#ifndef DEBUG_MODE_CLUSTER_NODE
#define DEBUG_MODE_CLUSTER_NODE 0
#endif

#ifndef DEBUG_MODE_TRIANGLE
#define DEBUG_MODE_TRIANGLE 0
#endif

struct vs_output
{
    float4 position_cs : SV_POSITION;

#if DEBUG_MODE_CLUSTER
    uint cluster_id : CLUSTER_ID;
#elif DEBUG_MODE_CLUSTER_NODE
    uint cluster_node_id : CLUSTER_NODE_ID;
#elif DEBUG_MODE_TRIANGLE
    uint primitive_offset : PRIMITIVE_OFFSET;
#endif

    float2 texcoord : TEXCOORD;
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;
    uint cluster_id = draw_infos[draw_id].cluster_id;

    mesh mesh = mesh::create(instance_id, scene);
    vertex vertex = mesh.fetch_vertex(vertex_id, camera.matrix_vp);

    vs_output output;
    output.position_cs = vertex.position_cs;

#if DEBUG_MODE_CLUSTER
    output.cluster_id = cluster_id;
#elif DEBUG_MODE_CLUSTER_NODE
    if (cluster_id != 0xFFFFFFFF)
    {
        StructuredBuffer<cluster_data> clusters = ResourceDescriptorHeap[scene.cluster_buffer];
        cluster_data cluster = clusters[cluster_id];
        output.cluster_node_id = cluster.cluster_node;
    }
    else
    {
        output.cluster_node_id = 0;
    }
#elif DEBUG_MODE_TRIANGLE
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
#endif

    output.texcoord = vertex.texcoord;

    material_info material_info = load_material_info(scene.material_buffer, mesh.get_material_address());
    output.opacity_mask = material_info.opacity_mask;
    output.opacity_cutoff = material_info.opacity_cutoff;

    return output;
}

float4 fs_main(vs_output input, uint primitive_id : SV_PrimitiveID) : SV_Target0
{
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.texcoord).a;
    clip(mask * 255.0 - input.opacity_cutoff);

    float3 color;

#if DEBUG_MODE_CLUSTER
    color = to_color(input.cluster_id);
#elif DEBUG_MODE_CLUSTER_NODE
    color = to_color(input.cluster_node_id);
#elif DEBUG_MODE_TRIANGLE
    color = to_color(input.primitive_offset + primitive_id);
#endif

    return float4(color, 1.0);
}