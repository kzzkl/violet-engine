#include "common.hlsli"
#include "utils.hlsli"
#include "distance_field/sdf_common.hlsli"
#include "sdf_trace.hlsli"

struct constant_data
{
    uint mesh_index;
    uint mesh_buffer;

    uint distance_field_buffer;
    uint brick_table;
    uint brick_atlas;

    uint debug_output;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

float3 intersect_mesh(
    ray ray,
    mesh_sdf mesh,
    StructuredBuffer<distance_field> distance_fields,
    StructuredBuffer<uint> brick_table,
    Texture3D<float> brick_atlas)
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
    return hit.is_hit ? hit.position / distance_field.volume_extent + 0.5 : float3(0.0, 0.0, 0.0);
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

    mesh_sdf mesh = meshes[constant.mesh_index];

    ray ray = ray::create(dtid.xy, width, height, camera.position, camera.matrix_vp_inv);

    float3 color = intersect_mesh(ray, mesh, distance_fields, brick_table, brick_atlas);

    debug_output[dtid.xy] = float4(color, 1.0);
}