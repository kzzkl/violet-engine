#include "common.hlsli"
#include "gbuffer.hlsli" 

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

float get_outline_camera_fov_and_distance_fix_multiplier(float position_vs_z)
{
    float camera_mul_fix = abs(position_vs_z);
    camera_mul_fix = saturate(camera_mul_fix);
    camera_mul_fix *= camera.fov;
    return camera_mul_fix * 0.001;
}

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct vs_out
{
    float4 position_cs : SV_POSITION;
    float3 color : COLOR;
};

struct mmd_outline
{
    float3 color;
    float width;
};

vs_out vs_main(vs_in input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];
    
    mmd_outline material = load_material<mmd_outline>(scene.material_buffer, instance.material_address);

    float4 position_ws = mul(mesh.model_matrix, float4(input.position, 1.0));
    float4 position_vs = mul(camera.view, position_ws);

    float3 normal_ws = mul((float3x3)mesh.model_matrix, input.normal);

    float outline_expand = material.width * get_outline_camera_fov_and_distance_fix_multiplier(position_vs.z);
    position_ws.xyz += normal_ws * outline_expand;

    vs_out output;
    output.position_cs = mul(camera.view_projection, position_ws);
    output.color = material.color;

    return output;
}

gbuffer::packed fs_main(vs_out input)
{
    gbuffer::data gbuffer_data;
    gbuffer_data.albedo = input.color;
    gbuffer_data.roughness = 1.0;
    gbuffer_data.metallic = 0.0;
    gbuffer_data.emissive = 0.0;

    return gbuffer::pack(gbuffer_data);
}