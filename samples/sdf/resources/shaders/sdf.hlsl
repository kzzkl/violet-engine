#include "common.hlsli"

struct constant_data
{
    float3 bounding_box_min;
    uint render_target;
    float3 bounding_box_max;
    uint depth_buffer;

    // The SDF is sampled in the local space of the bounding box, so the pass
    // pushes the inverse of matrix_m (the local to world matrix).
    float4x4 matrix_m_inv;

    uint sdf;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

static const uint SDF_MARCH_STEPS = 128;
// The SDF is signed: negative inside, positive outside, zero on the surface.
// The surface is a zero crossing, so a small epsilon is an exact hit test.
static const float SDF_HIT_THRESHOLD = 0.005;
static const float SDF_MIN_STEP = 0.001;

// The engine uses a reversed depth range: the near plane maps to 1 and the far
// plane maps to 0. The background therefore keeps the far plane value.
static const float DEPTH_FAR = 0.0;

// The SDF texture covers the bounding box: its local space in [0, 1] per axis.
float3 get_sdf_uvw(float3 position_ls)
{
    float3 bounding_box_extent = constant.bounding_box_max - constant.bounding_box_min;
    return (position_ls - constant.bounding_box_min) / bounding_box_extent;
}

float sample_sdf(Texture3D<float> sdf, float3 uvw)
{
    return sdf.SampleLevel(get_linear_clamp_sampler(), uvw, 0.0);
}

// Ray / AABB intersection (slab method).
// Returns (enter, exit) distances along the ray; exit < enter when no hit.
float2 intersect_aabb(float3 origin, float3 direction, float3 box_min, float3 box_max)
{
    float3 inv_direction = 1.0 / direction;

    float3 t0 = (box_min - origin) * inv_direction;
    float3 t1 = (box_max - origin) * inv_direction;

    float3 t_enter = min(t0, t1);
    float3 t_exit = max(t0, t1);

    return float2(
        max(max(t_enter.x, t_enter.y), t_enter.z),
        min(min(t_exit.x, t_exit.y), t_exit.z));
}

// Surface normal from the gradient of the distance field (central differences).
// For a signed field the gradient points outward, i.e. it is the surface normal.
float3 get_sdf_normal(Texture3D<float> sdf, float3 uvw)
{
    const float eps = 1.0 / 64.0; // One voxel in texture space.

    float3 normal;
    normal.x = sample_sdf(sdf, uvw + float3(eps, 0.0, 0.0)) -
               sample_sdf(sdf, uvw - float3(eps, 0.0, 0.0));
    normal.y = sample_sdf(sdf, uvw + float3(0.0, eps, 0.0)) -
               sample_sdf(sdf, uvw - float3(0.0, eps, 0.0));
    normal.z = sample_sdf(sdf, uvw + float3(0.0, 0.0, eps)) -
               sample_sdf(sdf, uvw - float3(0.0, 0.0, eps));

    return normalize(normal);
}

float3 aces_tonemap(float3 color)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return saturate((color * (a * color + b)) / (color * (c * color + d) + e));
}

float get_reverse_depth(float3 position_ws)
{
    float4 position_cs = mul(camera.matrix_vp, float4(position_ws, 1.0));

    if (position_cs.w <= 0.0)
    {
        return DEPTH_FAR;
    }

    return saturate(position_cs.z / position_cs.w);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> render_target = ResourceDescriptorHeap[constant.render_target];
    RWTexture2D<float> depth_buffer = ResourceDescriptorHeap[constant.depth_buffer];

    uint width;
    uint height;
    render_target.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    Texture3D<float> sdf = ResourceDescriptorHeap[constant.sdf];

    // Build the view ray in world space.
    float2 uv = get_compute_uv(dtid.xy, width, height);
    uv.y = 1.0 - uv.y;
    float4 ndc = float4(uv * 2.0 - 1.0, 0.0, 1.0);

    float3 ray_origin_ws = camera.position;
    float3 ray_direction_ws = normalize(mul(camera.matrix_vp_inv, ndc).xyz);

    float3 box_min = constant.bounding_box_min;
    float3 box_max = constant.bounding_box_max;

    // The bounding box is the local space of the SDF and matrix_m maps it to
    // world space. The field is only a distance field in its own space, so the
    // ray is brought there and marched in local units.
    float3x3 matrix_m_inv_3x3 = (float3x3)constant.matrix_m_inv;
    float3 ray_origin_ls = mul(constant.matrix_m_inv, float4(ray_origin_ws, 1.0)).xyz;
    float3 ray_direction_ls_unnormalized = mul(matrix_m_inv_3x3, ray_direction_ws);
    float3 ray_direction_ls = normalize(ray_direction_ls_unnormalized);

    // Background: dark gradient based on the ray direction.
    float3 color = float3(0.015, 0.02, 0.04) * (0.5 + 0.5 * ray_direction_ws.y);

    float2 t_range = intersect_aabb(ray_origin_ls, ray_direction_ls, box_min, box_max);

    // Pixels without a surface hit stay on the far plane.
    float depth = DEPTH_FAR;

    if (t_range.x <= t_range.y)
    {
        // Sphere trace: advance by the signed distance and stop at the zero
        // crossing (the first sample below the epsilon is the surface).
        float t = max(t_range.x, 0.0);
        float3 hit_uvw = 0.0;
        float hit_t = t;
        bool hit = false;

        [loop]
        for (uint i = 0; i < SDF_MARCH_STEPS; ++i)
        {
            float3 position_ls = ray_origin_ls + ray_direction_ls * t;
            float3 uvw = get_sdf_uvw(position_ls);
            float distance = sample_sdf(sdf, uvw);

            if (distance < SDF_HIT_THRESHOLD)
            {
                hit = true;
                hit_uvw = uvw;
                hit_t = t;
                break;
            }

            t += max(distance, SDF_MIN_STEP);
            if (t > t_range.y)
            {
                break;
            }
        }

        if (hit)
        {
            // hit_t is a local space distance: the world space hit position the
            // depth buffer needs is reached by the matching world distance.
            float hit_t_ws = hit_t / length(ray_direction_ls_unnormalized);
            float3 hit_position_ws = ray_origin_ws + ray_direction_ws * hit_t_ws;
            depth = get_reverse_depth(hit_position_ws);

            // The gradient of the field is in local space, the inverse transpose
            // of the matrix brings the normal to world space, where the light is.
            float3 normal_ls = get_sdf_normal(sdf, hit_uvw);
            float3 normal = normalize(mul(transpose(matrix_m_inv_3x3), normal_ls));

            float3 view_direction = -ray_direction_ws;
            if (dot(normal, view_direction) < 0.0)
            {
                normal = -normal;
            }

            float3 light_direction = normalize(float3(0.4, 0.8, -0.5));
            float3 half_vector = normalize(light_direction + view_direction);

            float diffuse = max(dot(normal, light_direction), 0.0);
            float specular = pow(max(dot(normal, half_vector), 0.0), 64.0);

            float3 albedo = float3(0.72, 0.52, 0.32); // Bronze.
            color = albedo * (0.12 + 0.88 * diffuse) + specular * 0.6;
        }
        else
        {
            // The ray crossed the volume but missed the surface:
            // brighten the background a little to reveal the volume bounds.
            color += float3(0.01, 0.02, 0.03);
        }
    }

    depth_buffer[dtid.xy] = depth;
    render_target[dtid.xy] = float4(aces_tonemap(color), 1.0);
}