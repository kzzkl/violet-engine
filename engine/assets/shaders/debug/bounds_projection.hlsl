#include "common.hlsli"
#include "utils.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct vs_output
{
    float4 aabb : AABB;
    uint visible : VISIABLE;
};

vs_output vs_main(uint vertex_id : SV_VertexID)
{
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    instance_data instance = instances[vertex_id];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    geometry_data geometry = geometries[instance.geometry_index];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh = meshes[instance.mesh_index];

    float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
    sphere_vs.w = geometry.bounding_sphere.w * mesh.scale.w;

    bool visible = true;

    vs_output output;
    if (camera.type == CAMERA_ORTHOGRAPHIC)
    {
        output.visible = project_shpere_orthographic(sphere_vs, camera.matrix_p[0][0], camera.matrix_p[1][1], camera.near, output.aabb);
    }
    else
    {
        output.visible = project_shpere_perspective(sphere_vs, camera.matrix_p[0][0], camera.matrix_p[1][1], camera.near, output.aabb);
    }

    return output;
}

struct gs_output
{
    float4 position_cs : SV_POSITION;
};

[maxvertexcount(8)]
void gs_main(point vs_output input[1], inout LineStream<gs_output> stream)
{
    if (!input[0].visible)
    {
        return;
    }

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