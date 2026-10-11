#include "common.hlsli"
#include "utils.hlsli"
#include "distance_field/sdf_common.hlsli"
#include "surface_cache/surface_cache_common.hlsli"
#include "sdf_trace.hlsli"

struct constant_data
{
    uint mesh_index;
    uint mesh_buffer;

    uint distance_field_buffer;
    uint brick_table;
    uint brick_atlas;

    uint surface_cache_buffer;
    uint surface_cache_albedo;
    uint surface_cache_depth;

    uint debug_output;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

struct surface_cache_data
{
    float3 albedo;
};

float2 get_surface_cache_uv(
    surface_cache surface_cache,
    uint face,
    float3 volume_uv)
{
    float3 x_axis[SURFACE_CACHE_FACE_COUNT] = {
        float3(0.0, 0.0, 0.5),
        float3(0.0, 0.0, -0.5),
        float3(-0.5, 0.0, 0.0),
        float3(-0.5, 0.0, 0.0),
        float3(-0.5, 0.0, 0.0),
        float3(0.5, 0.0, 0.0),
    };

    float3 y_axis[SURFACE_CACHE_FACE_COUNT] = {
        float3(0.0, 0.5, 0.0),
        float3(0.0, 0.5, 0.0),
        float3(0.0, 0.0, -0.5),
        float3(0.0, 0.0, 0.5),
        float3(0.0, 0.5, 0.0),
        float3(0.0, 0.5, 0.0),
    };

    float2 uv = float2(0.5 + dot(volume_uv, x_axis[face]), 0.5 - dot(volume_uv, y_axis[face]));

    uint2 extent = surface_cache.get_extent();
    uint2 page_count = surface_cache.get_page_count();

    float2 texel_f = min(uv * float2(extent), extent - 1);
    uint2 texel = floor(texel_f);
    uint2 page_coord = min(texel / SURFACE_CACHE_PAGE_RESOLUTION, page_count - 1);
    uint2 page_offset = texel - page_coord * SURFACE_CACHE_PAGE_RESOLUTION;

    uint2 atlas_texel =
        surface_cache.get_page(page_coord.y * page_count.x + page_coord.x) + page_offset;

    return (float2(atlas_texel) + texel_f - texel + 0.5) / SURFACE_CACHE_ATLAS_RESOLUTION;
}

uint get_surface_cache_face(
    uint surface_cache_id,
    float3 volume_uv,
    StructuredBuffer<surface_cache> surface_caches,
    Texture2D<float> surface_cache_depth)
{
    SamplerState point_clamp_sampler = get_point_clamp_sampler();

    uint result = 0xFFFFFFFF;
    float min_error = 10000.0;

    uint3 faces;
    faces[0] = volume_uv.x >= 0.0 ? 0 : 1;
    faces[1] = volume_uv.y >= 0.0 ? 2 : 3;
    faces[2] = volume_uv.z >= 0.0 ? 4 : 5;

    for (uint i = 0; i < 3; ++i)
    {
        uint face = faces[i];

        float2 uv = get_surface_cache_uv(surface_caches[surface_cache_id + face], face, volume_uv);

        float depth = surface_cache_depth.SampleLevel(point_clamp_sampler, uv, 0.0) * 2.0 - 1.0;
        float point_depth = face % 2 == 0 ? -volume_uv[face / 2] : volume_uv[face / 2];

        float error = abs(depth - point_depth);

        if (error < min_error)
        {
            result = face;
            min_error = error;
        }
    }

    return result;
}

surface_cache_data get_surface_cache(
    uint surface_cache_id,
    float3 volume_uv,
    StructuredBuffer<surface_cache> surface_caches,
    Texture2D<float4> surface_cache_albedo,
    Texture2D<float> surface_cache_depth)
{
    SamplerState linear_clamp_sampler = get_linear_clamp_sampler();

    float3 debug_color[6] = {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0),
        float3(1.0, 1.0, 0.0),
        float3(1.0, 0.5, 0.0),
        float3(1.0, 1.0, 1.0),
    };

    uint face = get_surface_cache_face(surface_cache_id, volume_uv, surface_caches, surface_cache_depth);
    float2 uv = get_surface_cache_uv(surface_caches[surface_cache_id + face], face, volume_uv);

    surface_cache_data result;
    result.albedo = surface_cache_albedo.SampleLevel(linear_clamp_sampler, uv, 0.0).rgb;
    // result.albedo = debug_color[face];

    return result;
}

float3 intersect_mesh(
    ray ray,
    mesh_sdf mesh,
    StructuredBuffer<distance_field> distance_fields,
    StructuredBuffer<uint> brick_table,
    Texture3D<float> brick_atlas,
    StructuredBuffer<surface_cache> surface_caches,
    Texture2D<float4> surface_cache_albedo,
    Texture2D<float> surface_cache_depth)
{
    float3 ray_start = mul(mesh.world_to_volume, float4(ray.origin, 1.0)).xyz;
    float3 ray_end = mul(mesh.world_to_volume, float4(ray.origin + ray.direction, 1.0)).xyz;

    ray.origin = ray_start;
    ray.direction = normalize(ray_end - ray_start);

    distance_field distance_field = distance_fields[mesh.distance_field_id];

    float3 volume_min = -distance_field.volume_extent * 0.5;
    float3 volume_max = -volume_min;

    ray_interval interval = ray.intersect_aabb(volume_min, volume_max);
    if (!interval.is_hit())
    {
        return 0.0;
    }

    ray_hit hit = sdf_trace(ray, interval, distance_field, brick_table, brick_atlas);

    if (!hit.is_hit || mesh.surface_cache_id == 0xFFFFFFFF)
    {
        return 0.0;
    }

    return get_surface_cache(
        mesh.surface_cache_id,
        hit.position,
        surface_caches,
        surface_cache_albedo,
        surface_cache_depth).albedo;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    RWTexture2D<float4> debug_output = ResourceDescriptorHeap[constant.debug_output];
    
    uint width;
    uint height;
    debug_output.GetDimensions(width, height);

    if (dtid.x >= width || dtid.y >= height)
    {
        return;
    }

    StructuredBuffer<mesh_sdf> meshes = ResourceDescriptorHeap[constant.mesh_buffer];
    StructuredBuffer<distance_field> distance_fields = ResourceDescriptorHeap[constant.distance_field_buffer];
    StructuredBuffer<uint> brick_table = ResourceDescriptorHeap[constant.brick_table];
    Texture3D<float> brick_atlas = ResourceDescriptorHeap[constant.brick_atlas];

    StructuredBuffer<surface_cache> surface_caches = ResourceDescriptorHeap[constant.surface_cache_buffer];
    Texture2D<float4> surface_cache_albedo = ResourceDescriptorHeap[constant.surface_cache_albedo];
    Texture2D<float> surface_cache_depth = ResourceDescriptorHeap[constant.surface_cache_depth];

    mesh_sdf mesh = meshes[constant.mesh_index];

    ray ray = ray::create(dtid.xy, width, height, camera.position, camera.matrix_vp_inv);

    float3 color = intersect_mesh(
        ray,
        mesh,
        distance_fields,
        brick_table,
        brick_atlas,
        surface_caches,
        surface_cache_albedo,
        surface_cache_depth);

    debug_output[dtid.xy] = float4(color, 1.0);
}