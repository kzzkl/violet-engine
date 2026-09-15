#include "shader_configs/material_config.hlsli"
#include "shader_configs/shading_model_config.hlsli"
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

#ifdef VIOLET_OPACITY_CUTOFF
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
#endif
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id, scene, vertex_id);

    vs_output output;
    output.material_address = mesh.get_material_address();

    material_data material = load_material<material_data>(scene.material_buffer, output.material_address);

#ifdef VIOLET_OPACITY_CUTOFF
    output.opacity_mask = material.common.get_opacity_mask();
    output.opacity_cutoff = material.common.get_opacity_cutoff();
#endif

    material_context context;
    context.scene = scene;
    context.camera = camera;

    output.varying = material.data.evaluate_varying(context, mesh);

    return output;
}

float4 fs_main(vs_output input) : SV_Target
{
#ifdef VIOLET_OPACITY_CUTOFF
    Texture2D<float4> opacity_mask = ResourceDescriptorHeap[input.opacity_mask];
    SamplerState point_repeat_sampler = get_point_repeat_sampler();

    float mask = opacity_mask.Sample(point_repeat_sampler, input.varying.uv).a;
    clip(mask * 255.0 - input.opacity_cutoff);
#endif

    material material_data = load_material_data<material>(scene.material_buffer, input.material_address);

    material_context material_context;
    material_context.scene = scene;
    material_context.camera = camera;
    surface surface = material_data.evaluate_surface(material_context, input.varying);

    shading_context shading_context;

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];
    for (int i = 0; i < scene.light_count; ++i)
    {
        light_data light = lights[i];

        if (light.cast_shadow)
        {
        }
    }

    return float4(0.0, 0.0, 0.0, 1.0);
}