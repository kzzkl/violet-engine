#include "mesh.hlsli"
#include "shading/shading_model.hlsli"

struct constant_data
{
    uint draw_info_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

uint murmur_mix(uint hash){
    hash ^= hash >> 16;
    hash *= 0x85ebca6b;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35;
    hash ^= hash >> 16;
    return hash;
}

float3 to_color(uint id)
{
    uint hash = murmur_mix(id + 1);

    float3 color = float3(
        (hash >> 0) & 255,
        (hash >> 8) & 255,
        (hash >> 16) & 255
    );

    return color * (1.0 / 255.0);
}

struct vs_output
{
    float4 position_cs : SV_POSITION;
    float3 normal_ws : NORMAL_WS;
    float3 tangent_ws : TANGENT_WS;
    float2 texcoord : TEXCOORD;
    uint material_address : MATERIAL_ADDRESS;
    uint cluster_id : CLUSTER_ID;
    float ndc_x : NDC_X;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint draw_id : SV_InstanceID)
{
    StructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
    uint instance_id = draw_infos[draw_id].instance_id;

    mesh mesh = mesh::create(instance_id, scene);
    vertex vertex = mesh.fetch_vertex(vertex_id, camera.matrix_vp);

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.normal_ws = vertex.normal_ws;
    output.tangent_ws = vertex.tangent_ws;
    output.texcoord = vertex.texcoord;
    output.material_address = mesh.get_material_address();
    output.cluster_id = draw_infos[draw_id].cluster_id;
    output.ndc_x = (vertex.position_cs.x / vertex.position_cs.w) * 0.5 + 0.5;

    return output;
}

struct fs_output
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    uint normal : SV_TARGET2;
    float4 emissive : SV_TARGET3;
};

struct pbr_material
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

fs_output shading_pbr(vs_output input)
{
    pbr_material material = load_material<pbr_material>(scene.material_buffer, input.material_address);

    SamplerState linear_repeat_sampler = get_linear_repeat_sampler();

    Texture2D<float3> albedo_texture = ResourceDescriptorHeap[material.albedo_texture];
    Texture2D<float3> roughness_metallic_texture = ResourceDescriptorHeap[material.roughness_metallic_texture];
    float3 roughness_metallic = roughness_metallic_texture.Sample(linear_repeat_sampler, input.texcoord);

    Texture2D<float3> emissive_texture = ResourceDescriptorHeap[material.emissive_texture];
    float3 emissive = emissive_texture.Sample(linear_repeat_sampler, input.texcoord);

    float3 N = normalize(input.normal_ws);
    if (material.normal_texture != 0)
    {
        Texture2D<float3> normal_texture = ResourceDescriptorHeap[material.normal_texture];

        float3 tangent_normal = normalize(normal_texture.Sample(linear_repeat_sampler, input.texcoord) * 2.0 - 1.0);
        float3 n = normalize(input.normal_ws);
        float3 t = normalize(input.tangent_ws);
        float3 b = normalize(cross(n, t));
        float3x3 tbn = transpose(float3x3(t, b, n));
        N = normalize(mul(tbn, tangent_normal));
    }
    
    material_info material_info = load_material_info(scene.material_buffer, input.material_address);

    fs_output output;
    output.albedo = float4(material.albedo * albedo_texture.Sample(linear_repeat_sampler, input.texcoord), 1.0);
    output.material = float2(material.roughness * roughness_metallic.g, material.metallic * roughness_metallic.b);
    output.emissive = float4(material.emissive * emissive, 1.0);
    output.normal = pack_gbuffer_normal(N, material_info.shading_model);

    return output;
}

fs_output shading_cluster(vs_output input)
{
    material_info material_info = load_material_info(scene.material_buffer, input.material_address);

    fs_output output;
    output.albedo = float4(to_color(input.cluster_id), 1.0);
    output.material = float2(1.0, 0.0);
    output.emissive = 0.0;
    output.normal = pack_gbuffer_normal(float3(0.0, 1.0, 0.0), material_info.shading_model);

    return output;
}

fs_output shading_primitive(vs_output input, uint primitive_id)
{
    material_info material_info = load_material_info(scene.material_buffer, input.material_address);

    fs_output output;
    output.albedo = float4(to_color(primitive_id), 1.0);
    output.material = float2(1.0, 0.0);
    output.emissive = 0.0;
    output.normal = pack_gbuffer_normal(float3(0.0, 1.0, 0.0), material_info.shading_model);

    return output;
}

fs_output fs_main(vs_output input, uint primitive_id : SV_PrimitiveID)
{
    // if (input.ndc_x < 0.3333)
    // {
    //     return shading_primitive(input, primitive_id);
    // }
    // else if (input.ndc_x < 0.6666)
    // {
        return shading_pbr(input);
    // }
    // else
    // {
    //     return shading_cluster(input);
    // }
}