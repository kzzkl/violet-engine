#include "common.hlsli"
#include "gbuffer.hlsli" 

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_in
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct vs_out
{
    float4 position : SV_POSITION;
    float3 world_position : WORLD_POSITION;
    float3 world_normal : WORLD_NORMAL;
    float3 world_tangent : WORLD_TANGENT;
    float2 texcoord : TEXCOORD;
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
    output.world_tangent = mul((float3x3)mesh.model_matrix, input.tangent);
    output.texcoord = input.texcoord;
    output.material_address = instance.material_address;

    output.position = mul(camera.view_projection, float4(output.world_position, 1.0));

    return output;
}

struct physical_material
{
    float3 albedo;
    uint albedo_texture;
    float roughness;
    uint roughness_metallic_texture;
    float metallic;
    uint normal_texture;
    float3 emissive;
    uint emissive_texture;
};

float3 get_normal(vs_out input, Texture2D<float3> normal_texture)
{
    SamplerState linear_repeat_sampler = SamplerDescriptorHeap[scene.linear_repeat_sampler];
    float3 tangent_normal = normalize(normal_texture.Sample(linear_repeat_sampler, input.texcoord) * 2.0 - 1.0);

    float3 n = normalize(input.world_normal);
    float3 t = normalize(input.world_tangent);
    float3 b = normalize(cross(n, t));
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, tangent_normal));
}

gbuffer::packed fs_main(vs_out input)
{
    SamplerState linear_repeat_sampler = SamplerDescriptorHeap[scene.linear_repeat_sampler];

    physical_material material = load_material<physical_material>(scene.material_buffer, input.material_address);

    Texture2D<float3> albedo_texture = ResourceDescriptorHeap[material.albedo_texture];
    Texture2D<float3> roughness_metallic_texture = ResourceDescriptorHeap[material.roughness_metallic_texture];
    float3 roughness_metallic = roughness_metallic_texture.Sample(linear_repeat_sampler, input.texcoord);

    Texture2D<float3> emissive_texture = ResourceDescriptorHeap[material.emissive_texture];
    float3 emissive = emissive_texture.Sample(linear_repeat_sampler, input.texcoord);

    float3 normal = input.world_normal;
    if (material.normal_texture != 0)
    {
        Texture2D<float3> normal_texture = ResourceDescriptorHeap[material.normal_texture];
        normal = get_normal(input, normal_texture);
    }

    gbuffer::data gbuffer_data;

    gbuffer_data.albedo = material.albedo * albedo_texture.Sample(linear_repeat_sampler, input.texcoord);
    gbuffer_data.roughness = material.roughness * roughness_metallic.g;
    gbuffer_data.metallic = material.metallic * roughness_metallic.b;
    gbuffer_data.emissive = material.emissive * emissive;
    gbuffer_data.normal = normal;

    return gbuffer::pack(gbuffer_data);
}