#include "common.hlsli"
#include "gbuffer.hlsli" 

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_input
{
    float3 position : POSITION;
};

struct vs_output
{
    float4 position : SV_POSITION;
    uint material_address : MATERIAL_ADDRESS;
};

vs_output vs_main(vs_input input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];

    vs_output output;
    output.position = mul(camera.view_projection, mul(mesh.model_matrix, float4(input.position, 1.0)));
    output.material_address = instance.material_address;

    return output;
}

struct unlit_material
{
    float3 albedo;
};

gbuffer::packed fs_main(vs_output input)
{
    unlit_material material = load_material<unlit_material>(scene.material_buffer, input.material_address);

    gbuffer::data gbuffer_data;
    gbuffer_data.albedo = material.albedo;

    return gbuffer::pack(gbuffer_data);
}