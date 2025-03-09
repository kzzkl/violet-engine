#include "mesh.hlsli"

struct vs_output
{
    float4 position_cs : SV_POSITION;
    uint material_address : MATERIAL_ADDRESS;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    mesh mesh = mesh::create(instance_id, vertex_id);
    vertex vertex = mesh.fetch_vertex();

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.material_address = mesh.get_material_address();

    return output;
}

struct unlit_material
{
    float3 albedo;
};

struct fs_output
{
    float4 albedo : SV_TARGET0;
    float2 material : SV_TARGET1;
    float2 normal : SV_TARGET2;
    float4 emissive : SV_TARGET3;
};

fs_output fs_main(vs_output input)
{
    unlit_material material = load_material<unlit_material>(scene.material_buffer, input.material_address);

    fs_output output;
    output.albedo = float4(material.albedo, 1.0);
    output.material = 0.0;
    output.normal = 0.0;
    output.emissive = 0.0;

    return output;
}