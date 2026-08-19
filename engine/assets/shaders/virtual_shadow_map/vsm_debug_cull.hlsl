#include "material.hlsli"
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

    float2 uv : TEXCOORD;
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<vsm_draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint vsm_id = draw_infos[draw_id].vsm_id;
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id, scene, vertex_id);

    vs_output output;
    output.position_cs = mul(camera.matrix_vp, mul(mesh.get_model_matrix(), float4(mesh.vertex.position, 1.0)));
    output.position_ndc = output.position_cs.xyz / output.position_cs.w;
    output.vsm_id = vsm_id;

    output.uv = mesh.vertex.uv;

    material_common material = load_material<material_common>(scene.material_buffer, mesh.get_material_address());
    output.opacity_mask = material.get_opacity_mask();
    output.opacity_cutoff = material.get_opacity_cutoff();

    return output;
}

float4 fs_main(vs_output input) : SV_Target
{
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.uv).a;
    clip(mask * 255.0 - input.opacity_cutoff);

    return float4(to_color(input.vsm_id), 1.0);
}