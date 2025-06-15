#pragma once

#include "common.hlsli"
#include "utils.hlsli"

static const uint CLUSTER_CULL_GROUP_SIZE = 64;
static const uint MAX_NODE_PER_GROUP = 8;

struct draw_command
{
    uint index_count;
    uint instance_count;
    uint index_offset;
    uint vertex_offset;
    uint instance_offset;
};

struct dispatch_command
{
    uint x;
    uint y;
    uint z;
};

struct cluster_queue_state_data
{
    uint cluster_queue_rear;
    uint cluster_node_queue_front;
    uint cluster_node_queue_rear;
    uint cluster_node_queue_prev_rear;
};

struct cluster_data
{
    float4 bounding_sphere;
    float4 lod_bounds;
    float lod_error;
    uint index_offset;
    uint index_count;
    uint padding0;
};

struct cluster_node_data
{
    float4 bounding_sphere;
    float4 lod_bounds;
    float min_lod_error;
    float max_parent_lod_error;
    uint is_leaf;
    uint child_offset;
    uint child_count;
    uint padding0;
    uint padding1;
    uint padding2;
};

struct bounding_sphere_cull
{
    float4 sphere_vs;

    float near;
    float p00;
    float p11;

    static bounding_sphere_cull create(float4 sphere_ws, const camera_data camera)
    {
        bounding_sphere_cull result;
        result.sphere_vs = mul(camera.matrix_v, float4(sphere_ws.xyz, 1.0));
        result.sphere_vs.w = sphere_ws.w;
        result.near = camera.near;
        result.p00 = camera.matrix_p[0][0];
        result.p11 = camera.matrix_p[1][1];
        return result;
    }

    bool frustum_cull(float4 frustum)
    {
        bool visible = true;

        visible = visible && sphere_vs.z + sphere_vs.w > near;
        visible = visible && sphere_vs.z * frustum[1] - abs(sphere_vs.x) * frustum[0] > -sphere_vs.w;
        visible = visible && sphere_vs.z * frustum[3] - abs(sphere_vs.y) * frustum[2] > -sphere_vs.w;

        return visible;
    }

    bool occlusion_cull(Texture2D<float> hzb, SamplerState hzb_sampler, uint width, uint height)
    {
        if (sphere_vs.z - sphere_vs.w < near)
        {
            return true;
        }

        float4 aabb = project_shpere_vs(sphere_vs, p00, p11);
        aabb = aabb.xwzy * float4(0.5, -0.5, 0.5, -0.5) + 0.5;

        float aabb_width = abs(aabb.z - aabb.x) * width;
        float aabb_height = abs(aabb.w - aabb.y) * height;

        float level = floor(log2(max(aabb_width, aabb_height)));

        float depth = hzb.SampleLevel(hzb_sampler, (aabb.xy + aabb.zw) * 0.5, level);

        // Only works correctly on reverse depth projection matrices with an infinite far plane.
        float sphere_depth = near / (sphere_vs.z - sphere_vs.w);

        return sphere_depth >= depth;
    }
};