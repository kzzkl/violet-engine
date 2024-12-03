#include "common.hlsli"
#include "gbuffer.hlsli" 

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_position : POSITION;
    float3 world_normal : NORMAL;
    uint material_address : MATERIAL_ADDRESS;
};

vs_out vs_main(vs_in input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];

    vs_out output;
    output.world_position = mul(mesh.model_matrix, float4(input.position, 1.0)).xyz;
    output.world_normal = mul((float3x3)mesh.model_matrix, input.normal);
    output.position = mul(camera.view_projection, float4(output.world_position, 1.0));

    return output;
}

struct pbr_material
{
    float3 albedo;
    float roughness;
    float metallic;
    uint padding0;
    uint padding1;
    uint padding2;
};

gbuffer_packed fs_main(vs_out input)
{
    pbr_material material = load_material<pbr_material>(scene.material_buffer, input.material_address);
    return gbuffer_encode(material.albedo);
}