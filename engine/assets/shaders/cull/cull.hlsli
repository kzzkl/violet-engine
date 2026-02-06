#ifndef CULL_HLSLI
#define CULL_HLSLI

#include "common.hlsli"
#include "utils.hlsli"

#define CULL_STAGE_MAIN_PASS 0
#define CULL_STAGE_POST_PASS 1

bool frustum_cull(float4 sphere_vs, camera_data camera)
{
    bool visible = sphere_vs.z + sphere_vs.w > camera.near;

    if (camera.type == CAMERA_PERSPECTIVE)
    {
        visible = visible && sphere_vs.z * camera.frustum[1] - abs(sphere_vs.x) * camera.frustum[0] > -sphere_vs.w;
        visible = visible && sphere_vs.z * camera.frustum[3] - abs(sphere_vs.y) * camera.frustum[2] > -sphere_vs.w;
    }
    else
    {
        float half_height = camera.orthographic_size;
        float half_width = half_height * camera.aspect;

        visible = visible && (sphere_vs.x + sphere_vs.w) > -half_width && (sphere_vs.x - sphere_vs.w) < half_width;
        visible = visible && (sphere_vs.y + sphere_vs.w) > -half_height && (sphere_vs.y - sphere_vs.w) < half_height;
    }

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
        if (!project_shpere_orthographic(sphere_vs, matrix_p[0][0], matrix_p[1][1], aabb))
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

    if (camera_type == CAMERA_ORTHOGRAPHIC)
    {
        return (sphere_vs.z - sphere_vs.w) * matrix_p[2][2] + matrix_p[2][3] > depth;
    }
    else
    {
        // Only works correctly on reverse depth projection matrices with an infinite far plane.
        return near / (sphere_vs.z - sphere_vs.w) > depth;
    }
}

#endif