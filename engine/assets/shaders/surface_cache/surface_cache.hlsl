#include "shader_configs/material_config.hlsli"

struct constant_data
{
    float4x4 matrix_vp;

    uint submesh_id;

    uint geometry_buffer;
    uint vertex_buffer;

    uint material_buffer;
    uint material_address;
};
PushConstant(constant_data, constant);

struct vs_output
{
    material::varying varying;

    uint material_address : MATERIAL_ADDRESS;

#ifdef VIOLET_OPACITY_CUTOFF
    uint opacity_mask : OPACITY_MASK;
    uint opacity_cutoff : OPACITY_CUTOFF;
#endif
};

vs_output vs_main(uint vertex_id : SV_VertexID)
{
    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[constant.geometry_buffer];

    mesh mesh;
    mesh.geometry = geometries[constant.submesh_id];
    mesh.data.matrix_m = float4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    mesh.vertex_buffer_id = constant.vertex_buffer;
    mesh.vertex_id = vertex_id;

    vs_output output;
    output.material_address = constant.material_address;

    material_data material = load_material<material_data>(constant.material_buffer, constant.material_address);

#ifdef VIOLET_OPACITY_CUTOFF
    output.opacity_mask = material.common.get_opacity_mask();
    output.opacity_cutoff = material.common.get_opacity_cutoff();
#endif

    material_context context;
    context.camera.matrix_vp = constant.matrix_vp;
    output.varying = material.data.evaluate_varying(context, mesh);

    return output;
}

struct fs_output
{
    float4 albedo : SV_TARGET0;
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

    material material_data = load_material_data<material>(constant.material_buffer, input.material_address);

    surface surface = material_data.evaluate_surface(context, input.varying);

    fs_output output;
    output.albedo = float4(surface.albedo, 1.0);
    return output;
}