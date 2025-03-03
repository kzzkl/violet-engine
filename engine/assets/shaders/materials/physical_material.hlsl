#include "common.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_input
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
    float2 texcoord : TEXCOORD;
};

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float4 prev_position_cs : PREV_POSITION_WS;
    float3 position_ws : POSITION_WS;
    float3 normal_ws : NORMAL_WS;
    float3 tangent_ws : TANGENT_WS;
    float2 texcoord : TEXCOORD;
    uint material_address : MATERIAL_ADDRESS;
};

vs_output vs_main(vs_input input, uint instance_index : SV_InstanceID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];

    vs_output output;
    output.position_ws = mul(mesh.model_matrix, float4(input.position, 1.0)).xyz;
    output.normal_ws = mul((float3x3)mesh.model_matrix, input.normal);
    output.tangent_ws = mul((float3x3)mesh.model_matrix, input.tangent.xyz);
    output.texcoord = input.texcoord;
    output.material_address = instance.material_address;

    output.position_cs = mul(camera.view_projection, float4(output.position_ws, 1.0));
    output.prev_position_cs = mul(camera.prev_view_projection, float4(output.position_ws, 1.0));

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

float3 get_normal(vs_output input, Texture2D<float3> normal_texture)
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();
    float3 tangent_normal = normalize(normal_texture.Sample(linear_repeat_sampler, input.texcoord) * 2.0 - 1.0);

    float3 n = normalize(input.normal_ws);
    float3 t = normalize(input.tangent_ws);
    float3 b = normalize(cross(n, t));
    float3x3 tbn = transpose(float3x3(t, b, n));

    return normalize(mul(tbn, tangent_normal));
}

struct fs_output
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    float2 normal : SV_TARGET2;
    float4 emissive : SV_TARGET3;
};

fs_output fs_main(vs_output input)
{
    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

    physical_material material = load_material<physical_material>(scene.material_buffer, input.material_address);

    Texture2D<float3> albedo_texture = ResourceDescriptorHeap[material.albedo_texture];
    Texture2D<float3> roughness_metallic_texture = ResourceDescriptorHeap[material.roughness_metallic_texture];
    float3 roughness_metallic = roughness_metallic_texture.Sample(linear_repeat_sampler, input.texcoord);

    Texture2D<float3> emissive_texture = ResourceDescriptorHeap[material.emissive_texture];
    float3 emissive = emissive_texture.Sample(linear_repeat_sampler, input.texcoord);

    float3 N = input.normal_ws;
    if (material.normal_texture != 0)
    {
        Texture2D<float3> normal_texture = ResourceDescriptorHeap[material.normal_texture];
        N = get_normal(input, normal_texture);
    }

    fs_output output;

    output.albedo = float4(material.albedo * albedo_texture.Sample(linear_repeat_sampler, input.texcoord), 1.0);
    output.material.x = material.roughness * roughness_metallic.g;
    output.material.y = material.metallic * roughness_metallic.b;
    output.emissive = float4(material.emissive * emissive, 1.0);
    output.normal = normal_to_octahedron(N);

    return output;
}