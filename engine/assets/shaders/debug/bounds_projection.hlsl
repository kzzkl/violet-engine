#include "common.hlsli"
#include "utils.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 aabb : AABB;
};

vs_output vs_main(uint vertex_id : SV_VertexID)
{
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh = meshes[vertex_id];

    float4 sphere_vs = float4(mesh.bounding_sphere.xyz, 1.0);
    sphere_vs = mul(camera.matrix_v, sphere_vs);
    sphere_vs.w = mesh.bounding_sphere.w;

    vs_output output;
    output.aabb = project_shpere_vs(sphere_vs, camera.matrix_p[0][0], camera.matrix_p[1][1]);
    return output;
}

struct gs_output
{
    float4 position_cs : SV_POSITION;
};

[maxvertexcount(8)]
void gs_main(point vs_output input[1], inout LineStream<gs_output> stream)
{
    float4 aabb = input[0].aabb;

    gs_output output[4];
    output[0].position_cs = float4(aabb.x, aabb.y, 0.0, 1.0);
    output[1].position_cs = float4(aabb.x, aabb.w, 0.0, 1.0);
    output[2].position_cs = float4(aabb.z, aabb.w, 0.0, 1.0);
    output[3].position_cs = float4(aabb.z, aabb.y, 0.0, 1.0);

    stream.Append(output[0]);
    stream.Append(output[1]);
    stream.RestartStrip();
    
    stream.Append(output[1]);
    stream.Append(output[2]);
    stream.RestartStrip();

    stream.Append(output[2]);
    stream.Append(output[3]);
    stream.RestartStrip();

    stream.Append(output[3]);
    stream.Append(output[0]);
    stream.RestartStrip();
}

float4 fs_main() : SV_TARGET0
{
    return float4(1.0, 0.0, 0.0, 1.0);
}