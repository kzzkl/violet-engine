#ifndef SDF_TRACE_HLSLI
#define SDF_TRACE_HLSLI

#include "common.hlsli"
#include "ray.hlsli"
#include "distance_field/sdf_common.hlsli"

ray_hit sdf_trace(
    ray ray,
    ray_interval interval,
    distance_field distance_field,
    StructuredBuffer<uint> brick_table,
    Texture3D<float> brick_atlas)
{
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float surface_epsilon = 0.05 * distance_field.max_distance;

    ray_hit result = ray_hit_miss;

    float t = interval.enter;
    for (uint step = 0; step < 128; ++step)
    {
        if (t > interval.exit)
        {
            break;
        }

        result.position = ray.origin + ray.direction * t;

        float distance = distance_field.sample(
            result.position,
            brick_table,
            brick_atlas,
            linear_clamp_sampler);

        if (distance < surface_epsilon)
        {
            result.is_hit = true;
            break;
        }

        t += distance;
    }

    return result;
}
#endif