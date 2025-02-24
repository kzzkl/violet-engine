#include "gf2/gf2_material.hlsli"

vs_output vs_main(vs_input input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];

    vs_output output;
    output.position_ws = mul(mesh.model_matrix, float4(input.position, 1.0)).xyz;
    output.position = mul(camera.view_projection, float4(output.position_ws, 1.0));
    output.normal_ws = mul((float3x3)mesh.model_matrix, input.normal);
    output.tangent_ws = mul((float3x3)mesh.model_matrix, input.tangent);
    output.texcoord = input.texcoord;
    output.texcoord2 = input.texcoord2;
    output.material_address = instance.material_address;

    return output;
}