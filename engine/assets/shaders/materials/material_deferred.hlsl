#include "shader_configs/material_config.hlsli"
#include "gbuffer.hlsli"

struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    material::varying varying;

    uint material_address : MATERIAL_ADDRESS;
    uint shading_model : SHADING_MODEL;

#ifdef VIOLET_OPACITY_CUTOFF
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
#endif
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;

    material_context context;
    mesh mesh = mesh::create(instance_id, scene, vertex_id);

    vs_output output;
    output.material_address = mesh.get_material_address();

    material_data material = load_material<material_data>(scene.material_buffer, output.material_address);
    output.shading_model = material.common.get_shading_model();

#ifdef VIOLET_OPACITY_CUTOFF
    output.opacity_mask = material.common.get_opacity_mask();
    output.opacity_cutoff = material.common.get_opacity_cutoff();
#endif

    output.varying = material.data.evaluate_varying(context, mesh, camera);

    return output;
}

struct fs_output
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    uint normal : SV_TARGET2;
    float3 emissive : SV_TARGET3;
};

fs_output fs_main(vs_output input)
{
#ifdef VIOLET_OPACITY_CUTOFF
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.varying.uv).a;
    clip(mask * 255.0 - input.opacity_cutoff);
#endif

    material_context context;
    material material_data = load_material_data<material>(scene.material_buffer, input.material_address);

    surface surface = material_data.evaluate_surface(context, input.varying, camera);

    gbuffer gbuffer;
    gbuffer.albedo = surface.albedo;
    gbuffer.roughness = surface.roughness;
    gbuffer.metallic = surface.metallic;
    gbuffer.emissive = surface.emissive;
    gbuffer.normal = surface.normal_ws;
    gbuffer.shading_model = input.shading_model;

    fs_output output;
    gbuffer.pack(output.albedo, output.material, output.normal, output.emissive);

    return output;
}