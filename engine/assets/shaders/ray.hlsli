#ifndef RAY_HLSLI
#define RAY_HLSLI

struct ray_interval
{
    float enter;
    float exit;

    bool is_hit()
    {
        return enter < exit;
    }
};

struct ray_hit
{
    float3 position;
    float is_hit;
};

static const ray_interval ray_interval_miss = { 1.0, 0.0 };
static const ray_hit ray_hit_miss = { float3(0.0, 0.0, 0.0), false };

struct ray
{
	float3 origin;
	float3 direction;

    static ray create(uint2 screen_coord, uint screen_width, uint screen_height, float3 camera_position, float4x4 matrix_vp_inv)
    {
        ray ray;

        float2 uv = (float2(screen_coord) + 0.5) / float2(screen_width, screen_height);
        uv.y = 1.0 - uv.y;
        float4 ndc = float4(uv * 2.0 - 1.0, 0.0, 1.0);

        ray.origin = camera_position;
        ray.direction = normalize(mul(matrix_vp_inv, ndc).xyz);

        return ray;
    }

    ray_interval intersect_aabb(float3 aabb_min, float3 aabb_max)
    {
        float3 direction_inv = 1.0 / direction;

        float3 t0 = (aabb_min - origin) * direction_inv;
        float3 t1 = (aabb_max - origin) * direction_inv;

        float3 t_min = min(t0, t1);
        float3 t_max = max(t0, t1);

        ray_interval result;

        result.enter = max(max(t_min.x, t_min.y), t_min.z);
        result.exit  = min(min(t_max.x, t_max.y), t_max.z);

        if (result.exit < max(result.enter, 0.0))
        {
            return ray_interval_miss;
        }

        return result;
    }
};

#endif