#include "mesh.hlsli"

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float3 position_ws : POSITION_WS;
    float3 normal_ws : NORMAL_WS;
    float3 tangent_ws : TANGENT_WS;
    float2 texcoord : TEXCOORD;
    uint material_address : MATERIAL_ADDRESS;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    mesh mesh = mesh::create(instance_id, vertex_id);
    vertex vertex = mesh.fetch_vertex();

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.position_ws = vertex.position_ws;
    output.normal_ws = vertex.normal_ws;
    output.tangent_ws = vertex.tangent_ws;;
    output.texcoord = vertex.texcoord;
    output.material_address = mesh.get_material_address();

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
    // output.albedo = float4(input.texcoord, 0.0, 1.0);
    output.material.x = material.roughness * roughness_metallic.g;
    output.material.y = material.metallic * roughness_metallic.b;
    output.emissive = float4(material.emissive * emissive, 1.0);
    output.normal = normal_to_octahedron(N);

    return output;
}