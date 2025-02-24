#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_input
{
    float3 position : POSITION;
};

struct vs_output
{
    float4 position_cs : SV_POSITION;
};

vs_output vs_main(vs_input input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];

    vs_output output;
    float4 position_ws = mul(mesh.model_matrix, float4(input.position, 1.0));
    output.position_cs = mul(camera.view_projection, position_ws);

    return output;
}