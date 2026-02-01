#ifndef CLUSTER_HLSLI
#define CLUSTER_HLSLI

#include "common.hlsli"

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

float get_projected_error(camera_data camera, float error, float depth)
{
    error *= camera.pixels_per_unit;
    return camera.type == CAMERA_PERSPECTIVE ? error / depth : error;
}

struct cluster_data
{
    float4 bounding_sphere;
    float4 lod_bounds;
    float lod_error;
    uint index_offset;
    uint index_count;
    uint padding0;

    bool check_lod(camera_data camera, mesh_data mesh, float threshold)
    {
        float3 center = mul(camera.matrix_v, mul(mesh.matrix_m, float4(lod_bounds.xyz, 1.0))).xyz;
        float radius = lod_bounds.w * mesh.scale.w;

        float near = center.z - radius;
    
        // if camera inside lod sphere, use lod 0.
        if (near < camera.near)
        {
            return lod_error == -1.0;
        }

        float error = get_projected_error(camera, lod_error * mesh.scale.w, near);
        return error <= threshold;
    }
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

    bool check_lod(camera_data camera, mesh_data mesh, float threshold)
    {
        float3 center = mul(camera.matrix_v, mul(mesh.matrix_m, float4(lod_bounds.xyz, 1.0))).xyz;
        float radius = lod_bounds.w * mesh.scale.w;

        float near = center.z - radius;
        float far = center.z + radius;

        if (near < camera.near)
        {
            return true;
        }

        float min_error = get_projected_error(camera, min_lod_error * mesh.scale.w, far);
        float max_error = get_projected_error(camera, max_parent_lod_error * mesh.scale.w, near);

        return min_error <= threshold && threshold < max_error;
    }
};

#endif