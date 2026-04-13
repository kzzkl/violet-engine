#ifndef HZB_TRACE_HLSLI
#define HZB_TRACE_HLSLI

#include "common.hlsli"

bool hzb_trace(
    float3 ray_origin_ws,
    float3 ray_direction_ws,
    float thickness,
    uint iteration_count,
    Texture2D<float> hzb,
    float2 hzb_texel_size,
    float hzb_level_count,
    camera_data camera)
{
    float3 ray_origin_uv;
    float3 ray_direction_uv;
    {
        float4 ray_origin_cs = mul(camera.matrix_vp, float4(ray_origin_ws, 1.0));
        ray_origin_uv = ray_origin_cs.xyz / ray_origin_cs.w;
        ray_origin_uv.xy = ray_origin_uv.xy * float2(0.5, -0.5) + 0.5;

        float4 ray_end_cs = mul(camera.matrix_vp, float4(ray_origin_ws + ray_direction_ws, 1.0));
        float3 ray_end_uv = ray_end_cs.xyz / ray_end_cs.w;
        ray_end_uv.xy = ray_end_uv.xy * float2(0.5, -0.5) + 0.5;

        ray_direction_uv = ray_end_uv - ray_origin_uv;
    }

    float3 ray_start_uv = float3(ray_origin_uv.xy * float2(0.5, -0.5) + 0.5, ray_origin_uv.z);
    float3 ray_end_uv = ray_origin_uv + ray_direction_uv;

    float3 position_uv = ray_start_uv;

    float2 floor_offset = select(ray_direction_uv.xy < 0.0, 0.0, 1.0);

    float base_level = -1.0;
    float level = 0.0;

    {
        float2 level_texel_size = exp2(level) * hzb_texel_size;
        float2 level_resolution = 1.0 / level_texel_size;

        float2 boundary = floor(position_uv.xy * level_resolution) + floor_offset;

        float2 intersection_times = (boundary - ray_start_uv.xy) / ray_direction_uv.xy;
        float intersection_time = min(intersection_times.x, intersection_times.y);

        position_uv = ray_start_uv + ray_direction_uv * intersection_time;
    }

    float curr_intersection_time = 0.0;

    SamplerState point_clamp_sampler = get_point_clamp_sampler();

    uint iteration = 0;

    while (base_level <= level && level < hzb_level_count && iteration < iteration_count)
    {
        float2 level_texel_size = exp2(level) * hzb_texel_size;
        float2 level_resolution = 1.0 / level_texel_size;

        float3 boundary;
        boundary.xy = floor(position_uv.xy * level_resolution) + floor_offset;
        boundary.z = hzb.SampleLevel(point_clamp_sampler, position_uv.xy, level);

        float3 intersection_times = (boundary - ray_start_uv) / ray_direction_uv;
        intersection_times.z = ray_direction_uv.z < 0.0 ? 1.0 : intersection_times.z;
        float intersection_time = min(min(intersection_times.x, intersection_times.y), intersection_times.z);

        bool above_surface = position_uv.z > boundary.z;
        bool skip_tile = above_surface && intersection_time != intersection_times.z;

        if (skip_tile)
        {

        }

        curr_intersection_time = above_surface ? intersection_time : curr_intersection_time;
        position_uv = ray_start_uv + ray_direction_uv * curr_intersection_time;
        level += skip_tile ? 1.0 : -1.0;

        ++iteration;
    }

    bool hit = false;

    if (level < 0.0)
    {
        hit = true;
    }

    return hit;
}

#endif