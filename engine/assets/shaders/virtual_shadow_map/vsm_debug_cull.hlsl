#include "mesh.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"
#include "utils.hlsli"

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
    float3 position_ndc : POSITION_NDC;
    uint vsm_id : VSM_ID;

    float2 texcoord : TEXCOORD;
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<vsm_draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint vsm_id = draw_infos[draw_id].vsm_id;
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id, scene);
    vertex vertex = mesh.fetch_vertex(vertex_id, camera.matrix_vp);

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.position_ndc = vertex.position_cs.xyz / vertex.position_cs.w;
    output.vsm_id = vsm_id;

    output.texcoord = vertex.texcoord;

    material_info material_info = load_material_info(scene.material_buffer, mesh.get_material_address());
    output.opacity_mask = material_info.opacity_mask;
    output.opacity_cutoff = material_info.opacity_cutoff;

    return output;
}

float4 fs_main(vs_output input) : SV_Target
{
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.texcoord).a;
    clip(mask * 255.0 - input.opacity_cutoff);

    return float4(to_color(input.vsm_id), 1.0);
}