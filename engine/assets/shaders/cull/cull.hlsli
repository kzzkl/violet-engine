#pragma once

#include "common.hlsli"
#include "utils.hlsli"

#define CULL_STAGE_MAIN_PASS 0
#define CULL_STAGE_POST_PASS 1

static const uint CLUSTER_CULL_GROUP_SIZE = 64;
static const uint MAX_CLUSTER_NODE_PER_GROUP = 8;

struct cluster_queue_state_data
{
    uint cluster_queue_rear;
    uint cluster_node_queue_front;
    uint cluster_node_queue_rear;
    uint cluster_node_queue_prev_rear;

    uint cluster_recheck_size;
    uint cluster_node_recheck_size;
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

// Frustum culling for orthogonal projection.
bool frustum_cull(float4 sphere_vs, float width, float height, float near)
{
    float half_width = width * 0.5;
    float half_height = height * 0.5;

    bool visible = sphere_vs.z + sphere_vs.w > near;
    visible = visible && (sphere_vs.x + sphere_vs.w) > -half_width && (sphere_vs.x - sphere_vs.w) < half_width;
    visible = visible && (sphere_vs.y + sphere_vs.w) > -half_height && (sphere_vs.y - sphere_vs.w) < half_height;

    return visible;
}

// Frustum culling for perspective projection.
bool frustum_cull(float4 sphere_vs, float4 frustum, float near)
{
    bool visible = sphere_vs.z + sphere_vs.w > near;
    visible = visible && sphere_vs.z * frustum[1] - abs(sphere_vs.x) * frustum[0] > -sphere_vs.w;
    visible = visible && sphere_vs.z * frustum[3] - abs(sphere_vs.y) * frustum[2] > -sphere_vs.w;

    return visible;
}

bool occlusion_cull(
    float4 sphere_vs,
    Texture2D<float> hzb,
    SamplerState hzb_sampler,
    uint width,
    uint height,
    float4x4 matrix_p,
    float near,
    uint camera_type)
{
    if (sphere_vs.z - sphere_vs.w < near)
    {
        return true;
    }

    float4 aabb;
    
    if (camera_type == CAMERA_ORTHOGRAPHIC)
    {
        if (!project_shpere_orthographic(sphere_vs, matrix_p[0][0], matrix_p[1][1], near, aabb))
        {
            return false;
        }
    }
    else
    {
        if (!project_shpere_perspective(sphere_vs, matrix_p[0][0], matrix_p[1][1], near, aabb))
        {
            return false;
        }
    }

    aabb = aabb.xwzy * float4(0.5, -0.5, 0.5, -0.5) + 0.5;

    float aabb_width = abs(aabb.z - aabb.x) * width;
    float aabb_height = abs(aabb.w - aabb.y) * height;

    float level = floor(log2(max(aabb_width, aabb_height)));

    float depth = hzb.SampleLevel(hzb_sampler, (aabb.xy + aabb.zw) * 0.5, level);

    // if (camera_type == CAMERA_ORTHOGRAPHIC)
    // {
    //     return sphere_vs.z - sphere_vs.w > depth;
    // }
    // else
    {
        // Only works correctly on reverse depth projection matrices with an infinite far plane.
        float sphere_depth = near / (sphere_vs.z - sphere_vs.w);
        return sphere_depth > depth;
    }
}