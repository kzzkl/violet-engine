#ifndef HZB_TRACE_HLSLI
#define HZB_TRACE_HLSLI

#include "common.hlsli"

float4 hzb_trace(
    float3 ray_origin_ts,
    float3 ray_direction_ts,
    float thickness,
    uint iteration_count,
    Texture2D<float> hzb,
    float2 hzb_texel_size,
    int hzb_start_level,
    int hzb_end_level,
    float near_plane)
{
    {
        float3 boundary = select(ray_direction_ts < 0.0, 0.0, 1.0);
        float3 delta = (boundary - ray_origin_ts) / ray_direction_ts;
        float intersection_time = min(min(delta.x, delta.y), delta.z);

        ray_direction_ts *= intersection_time;
    }

    float2 floor_offset = select(ray_direction_ts.xy < 0, 0.0, 1.0);

    int level = hzb_start_level;

    float3 ray_ts;
    {
        float2 texel_size = exp2(level) * hzb_texel_size;
        float2 texel_count = 1.0 / texel_size;

        float2 boundary_offset = 0.005 * texel_size;
        boundary_offset = select(ray_direction_ts.xy < 0.0, -boundary_offset, boundary_offset);
        
        float2 boundary = floor(ray_origin_ts.xy * texel_count) + floor_offset;
        boundary = boundary * texel_size + boundary_offset;

        float2 delta = (boundary - ray_origin_ts.xy) / ray_direction_ts.xy;
        float intersection_time = min(delta.x, delta.y);

        ray_ts = ray_origin_ts + ray_direction_ts * intersection_time;
    }

    SamplerState point_clamp_sampler = get_point_clamp_sampler();

    uint iteration = 0;
    float total_intersection_time = 0.0;

    while (level >= hzb_start_level && iteration < iteration_count && total_intersection_time < 1.0)
    {
        float2 texel_size = exp2(level) * hzb_texel_size;
        float2 texel_count = 1.0 / texel_size;

        float depth = hzb.SampleLevel(point_clamp_sampler, ray_ts.xy, level);
        
        float2 boundary_offset = 0.005 * texel_size;
        boundary_offset = select(ray_direction_ts.xy < 0, -boundary_offset, boundary_offset);

        float3 boundary;
        boundary.xy = floor(ray_ts.xy * texel_count) + floor_offset;
        boundary.xy = boundary.xy * texel_size + boundary_offset;
        boundary.z = depth;

        float3 delta = (boundary - ray_ts) / ray_direction_ts;
        delta.z = ray_direction_ts.z < 0.0 ? delta.z : 1.0;
        float intersection_time = min(min(delta.x, delta.y), delta.z);

        if (ray_ts.z <= depth)
        {
            --level;
        }
        else
        {
            ray_ts += ray_direction_ts * intersection_time;
            total_intersection_time += intersection_time;
            level = min(level + 1, hzb_end_level);
        }

        ++iteration;
    }

    bool hit = false;
    if (level < hzb_start_level)
    {
        float depth = hzb.SampleLevel(point_clamp_sampler, ray_ts.xy, hzb_start_level);

        float delta = abs(near_plane / depth - near_plane / ray_ts.z);
        hit = delta <= thickness;
    }

    return float4(ray_ts, hit ? 1.0 : 0.0);
}

#endif