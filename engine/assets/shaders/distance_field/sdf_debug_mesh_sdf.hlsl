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

// The surface cache is baked by rendering the volume from six orthographic views, and the viewport
// covers the whole volume. A point of the volume therefore has the texture coordinate
// camera_space.xy / volume_extent + 0.5 on a face.
//
// look_at builds x_axis = normalize(cross(up, forward)) and y_axis = cross(forward, x_axis). The
// viewport maps the positive x of the camera to an increasing texture u and the positive y of the
// camera to a decreasing texture v, so a point of the volume has
// u = 0.5 + dot(position, x_axis) / volume_extent and v = 0.5 - dot(position, y_axis) /
// volume_extent. The corners of the table below are the uv of the point on each face, in the order
// of surface_cache_renderer::SURFACE_CACHE_FACES.
float2 get_surface_cache_uv(uint face, float3 volume_uv)
{
    float2 uvs[SURFACE_CACHE_FACE_COUNT];
    uvs[0] = float2(1.0 - volume_uv.y, volume_uv.z);            // +x
    uvs[1] = float2(volume_uv.y, volume_uv.z);                  // -x
    uvs[2] = float2(volume_uv.z, 1.0 - volume_uv.x);            // +y
    uvs[3] = float2(1.0 - volume_uv.x, volume_uv.z);            // -y
    uvs[4] = float2(1.0 - volume_uv.x, volume_uv.y);            // +z
    uvs[5] = float2(1.0 - volume_uv.x, volume_uv.y);            // -z

    return uvs[face];
}

// Selects the face whose view the hit surface faces, which is the face the surface cache recorded
// the surface from.
//
// The ray enters the mesh through the face that has the largest component of the direction on its
// axis, so the ray direction and the normal of the entry face point the opposite way. The surface
// it hits faces the ray, so its face is the one opposite to the entry face.
//
// This costs no sampling at all, but it is only right while the surface of the hit is also the
// closest surface to the camera that baked the face. A surface that another surface hides from
// that camera is not recorded on the face, so the albedo of the hit has to come from another face.
uint get_surface_cache_face_by_view(float3 direction)
{
    float3 abs_direction = abs(direction);

    uint axis = 0;
    if (abs_direction.y > abs_direction.x && abs_direction.y > abs_direction.z)
    {
        axis = 1;
    }
    else if (abs_direction.z > abs_direction.x && abs_direction.z > abs_direction.y)
    {
        axis = 2;
    }

    // The even faces look along the positive axes, the odd ones along the negative axes.
    return axis * 2 + (direction[axis] > 0.0 ? 0 : 1);
}

// The distance of a point to the near plane of the orthographic projection of a face. It is the
// distance the depth of the face measures.
//
// The camera of a face sits at the positive end of the axis the face looks along, so the near
// plane of the even faces (the positive ones, in the order of SURFACE_CACHE_FACES) is at the
// maximum of that axis and the near plane of the odd faces is at the minimum.
float get_surface_cache_depth_distance(uint face, float3 position, float3 volume_min, float3 volume_max)
{
    uint axis = face / 2;
    return (face % 2) == 0 ? volume_max[axis] - position[axis] : position[axis] - volume_min[axis];
}

// Reads the depth the face has recorded for a point of the volume, in the units of
// SURFACE_CACHE_DEPTH_RANGE. Returns 1.0 when the point is outside the face or when the face has
// nothing recorded for it.
float sample_surface_cache_depth(
    surface_cache surface_cache,
    uint face,
    float3 position,
    float3 volume_min,
    float3 volume_max,
    Texture2D<float> depth)
{
    float3 volume_uv = (position - (volume_min + volume_max) * 0.5) / (volume_max - volume_min) + 0.5;

    float2 uv = get_surface_cache_uv(face, volume_uv);
    if (any(uv < 0.0) || any(uv > 1.0))
    {
        return 1.0;
    }

    uint2 extent = surface_cache.get_extent();
    uint2 page_count = surface_cache.get_page_count();

    uint2 texel = min(uint2(uv * float2(extent)), extent - 1);
    uint2 page_coord = min(texel / SURFACE_CACHE_PAGE_RESOLUTION, page_count - 1);
    uint2 page_offset = texel - page_coord * SURFACE_CACHE_PAGE_RESOLUTION;

    uint2 atlas_texel =
        surface_cache.get_page(page_coord.y * page_count.x + page_coord.x) + page_offset;

    return depth.SampleLevel(
        get_point_clamp_sampler(),
        (float2(atlas_texel) + 0.5) / SURFACE_CACHE_ATLAS_RESOLUTION,
        0.0);
}

// Picks the face that has recorded the surface closest to the hit. A face knows a surface only
// where it is the closest surface to the camera of that face, so the recorded depth of the face
// that actually saw the hit matches the distance of the hit to the near plane of the face.
//
// The hit is at most one surface epsilon away from the real surface, which makes the error of the
// winning face non zero. A face whose error is far above that did not record the hit and is
// rejected, otherwise the noise of the trace flips the result between the faces that are almost
// equally close.
uint get_surface_cache_face(
    surface_cache surface_cache,
    float3 position,
    float3 volume_min,
    float3 volume_max,
    float error_tolerance,
    Texture2D<float> depth)
{
    uint result = 0xFFFFFFFF;
    float min_error = 0.0;

    for (uint face = 0; face < SURFACE_CACHE_FACE_COUNT; ++face)
    {
        float recorded = sample_surface_cache_depth(
            surface_cache,
            face,
            position,
            volume_min,
            volume_max,
            depth);

        // The face is behind the hit or has nothing recorded for it.
        if (recorded >= 1.0)
        {
            continue;
        }

        float distance = get_surface_cache_depth_distance(face, position, volume_min, volume_max);
        float error = abs(recorded * SURFACE_CACHE_DEPTH_RANGE - distance);

        if (error <= error_tolerance && (result == 0xFFFFFFFF || error < min_error))
        {
            result = face;
            min_error = error;
        }
    }

    return result;
}

float3 sample_surface_cache_albedo(
    surface_cache surface_cache,
    uint face,
    float3 position,
    float3 volume_min,
    float3 volume_max,
    Texture2D<float4> albedo)
{
    float3 volume_uv = (position - (volume_min + volume_max) * 0.5) / (volume_max - volume_min) + 0.5;

    float2 uv = saturate(get_surface_cache_uv(face, volume_uv));

    uint2 extent = surface_cache.get_extent();
    uint2 page_count = surface_cache.get_page_count();

    uint2 texel = min(uint2(uv * float2(extent)), extent - 1);
    uint2 page_coord = min(texel / SURFACE_CACHE_PAGE_RESOLUTION, page_count - 1);
    uint2 page_offset = texel - page_coord * SURFACE_CACHE_PAGE_RESOLUTION;

    uint2 atlas_texel =
        surface_cache.get_page(page_coord.y * page_count.x + page_coord.x) + page_offset;

    return albedo.SampleLevel(
        get_point_clamp_sampler(),
        (float2(atlas_texel) + 0.5) / SURFACE_CACHE_ATLAS_RESOLUTION,
        0.0).rgb;
}

static const float3 SURFACE_CACHE_FACE_COLOR[SURFACE_CACHE_FACE_COUNT] = {
    float3(1, 0, 0), float3(0, 1, 0), float3(0, 0, 1),
    float3(1, 1, 0), float3(1, 0, 1), float3(0, 1, 1),
};

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

    surface_cache surface_cache = surface_caches[mesh.surface_cache_id];

    // Selects the face from the direction of the ray. Swap in the depth based version below to
    // compare the two, it needs no ray direction but samples the depth of all six faces.
    uint face = get_surface_cache_face_by_view(ray.direction);

    // sdf_trace stops within one surface epsilon of the surface, which is 0.125 of the maximum
    // distance of the distance field.
    // float error_tolerance = distance_field.max_distance * 0.25;
    // uint face = get_surface_cache_face(
    //     surface_cache,
    //     hit.position,
    //     volume_min,
    //     volume_max,
    //     error_tolerance,
    //     surface_cache_depth);

    // return SURFACE_CACHE_FACE_COLOR[face];

    if (face == 0xFFFFFFFF)
    {
        // No face has a surface close to the hit.
        return float3(0.1, 0.0, 0.0);
    }

    return sample_surface_cache_albedo(
        surface_cache,
        face,
        hit.position,
        volume_min,
        volume_max,
        surface_cache_albedo);
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
