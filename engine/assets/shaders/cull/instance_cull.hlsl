#include "common.hlsli"
#include "utils.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct constant_data
{
    uint hzb;
    uint hzb_sampler;
    uint width;
    uint height;
    uint frustum_culling;
    uint occlusion_culling;
    uint count_buffer;
    uint command_buffer;
    float4 frustum;
};
PushConstant(constant_data, constant);

struct draw_command
{
    uint index_count;
    uint instance_count;
    uint index_offset;
    uint vertex_offset;
    uint instance_offset;
};

bool frustum_cull(float4 sphere_vs)
{
    bool visible = true;

    visible = visible && sphere_vs.z + sphere_vs.w > camera.near;
    visible = visible && sphere_vs.z * constant.frustum[1] - abs(sphere_vs.x) * constant.frustum[0] > -sphere_vs.w;
    visible = visible && sphere_vs.z * constant.frustum[3] - abs(sphere_vs.y) * constant.frustum[2] > -sphere_vs.w;

    return visible;
}

bool occlusion_cull(float4 sphere_vs)
{
    if (sphere_vs.z - sphere_vs.w < camera.near)
    {
        return true;
    }

    float4 aabb = project_shpere_vs(sphere_vs, camera.matrix_p[0][0], camera.matrix_p[1][1]);
    aabb = aabb.xwzy * float4(0.5, -0.5, 0.5, -0.5) + 0.5;

    float aabb_width = abs(aabb.z - aabb.x) * constant.width;
    float aabb_height = abs(aabb.w - aabb.y) * constant.height;

    float level = floor(log2(max(aabb_width, aabb_height)));

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];
    float depth = hzb.SampleLevel(hzb_sampler, (aabb.xy + aabb.zw) * 0.5, level);

    // Only works correctly on reverse depth projection matrices with an infinite far plane.
    float sphere_depth = camera.near / (sphere_vs.z - sphere_vs.w);

    return sphere_depth >= depth;
}

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint instance_index = dtid.x;
    if (instance_index >= scene.instance_count)
    {
        return;
    }

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    instance_data instance = instances[instance_index];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    geometry_data geometry = geometries[instance.geometry_index];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh = meshes[instance.mesh_index];

    float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
    sphere_vs.w = geometry.bounding_sphere.w;

    if (!frustum_cull(sphere_vs) || !occlusion_cull(sphere_vs))
    {
        return;
    }

    ByteAddressBuffer batch_buffer = ResourceDescriptorHeap[scene.batch_buffer];
    RWBuffer<uint> count_buffer = ResourceDescriptorHeap[constant.count_buffer];

    uint batch_index = instance.batch_index;

    uint command_index = 0;
    InterlockedAdd(count_buffer[batch_index], 1, command_index);
    command_index += batch_buffer.Load<uint>(batch_index * 4);

    draw_command command;
    command.index_count = geometry.index_count;
    command.instance_count = 1;
    command.index_offset = geometry.index_offset;
    command.vertex_offset = 0;
    command.instance_offset = instance_index;

    RWStructuredBuffer<draw_command> command_buffer = ResourceDescriptorHeap[constant.command_buffer];
    command_buffer[command_index] = command;
}