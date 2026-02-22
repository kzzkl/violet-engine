#include "cluster.hlsli"
#include "mesh.hlsli"
#include "visibility/visibility_utils.hlsli"

struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 position_cs : SV_POSITION;
    uint instance_id : INSTANCE_ID;
    uint primitive_offset : PRIMITIVE_OFFSET;

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

    output.texcoord = vertex.texcoord;

    material_info material_info = load_material_info(scene.material_buffer, mesh.get_material_address());
    output.opacity_mask = material_info.opacity_mask;
    output.opacity_cutoff = material_info.opacity_cutoff;

    return output;
}

uint2 fs_main(vs_output input, uint primitive_id : SV_PrimitiveID) : SV_Target0
{
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.texcoord).a;
    clip(mask * 255.0 - input.opacity_cutoff);

    return pack_visibility(input.instance_id, input.primitive_offset + primitive_id);
}