#include "mesh.hlsli"

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
    float3 color : COLOR;
    float3 normal_ws : NORMAL;
};

vs_output vs_main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    mesh mesh = mesh::create(instance_id, vertex_id);
    vertex vertex = mesh.fetch_vertex();

    vs_output output;
    output.position_cs = vertex.position_cs;
    output.color = to_color(instance_id);
    output.normal_ws = vertex.normal_ws;

    return output;
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
    float3 color = input.color * saturate(abs(dot(input.normal_ws, normalize(float3(1.0, 1.0, 0.0)))) + 0.2);

    fs_output output;
    output.albedo = float4(color, 1.0);
    output.material = 0.0;
    output.normal = 0.0;
    output.emissive = 0.0;

    return output;
}