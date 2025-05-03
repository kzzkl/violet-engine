#include "common.hlsli"
#include "utils.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct constant_data
{
    uint hzb;
    uint hzb_sampler;
    uint cull_result;
    uint width;
    uint height;
    uint frustum_culling;
    uint occlusion_culling;
    uint padding0;
    float4 frustum;
};
PushConstant(constant_data, constant);

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
    uint mesh_index = dtid.x;
    if (mesh_index >= scene.mesh_count)
    {
        return;
    }

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];

    mesh_data mesh = meshes[mesh_index];

    float4 sphere_vs = mul(camera.matrix_v, float4(mesh.bounding_sphere.xyz, 1.0));
    sphere_vs.w = mesh.bounding_sphere.w;

    bool visible = true;
    visible = visible && frustum_cull(sphere_vs);
    visible = visible && occlusion_cull(sphere_vs);

    RWByteAddressBuffer cull_result = ResourceDescriptorHeap[constant.cull_result];
    cull_result.Store(mesh_index * 4, visible);
}