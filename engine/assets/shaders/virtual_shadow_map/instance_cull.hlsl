#include "common.hlsli"
#include "cluster.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"
#include "utils.hlsli"

struct constant_data
{
    uint visible_light_count;
    uint visible_vsm_ids;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_page_table;
    uint vsm_bounds_buffer;
    uint hzb;
    uint hzb_sampler;
    uint draw_buffer;
    uint draw_count_buffer;
    uint draw_info_buffer;
    uint cluster_queue;
    uint cluster_queue_state;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);

bool overlap_required_page(uint vsm_id, uint4 required_page_bounds, float4 sphere_vs, vsm_data vsm)
{
    float4 projected_aabb;
    if (!project_shpere_orthographic(sphere_vs, vsm.matrix_p[0][0], vsm.matrix_p[1][1], projected_aabb))
    {
        return false;
    }

    // projected_aabb = projected_aabb.xwzy * float4(0.5, -0.5, 0.5, -0.5) + 0.5;
    projected_aabb = projected_aabb * 0.5 + 0.5;

    float4 projected_page_bounds = projected_aabb * VIRTUAL_PAGE_TABLE_SIZE;
    uint4 overlap_page_bounds;
    overlap_page_bounds.xy = max(0.0, floor(projected_page_bounds.xy));
    overlap_page_bounds.zw = max(0.0, ceil(projected_page_bounds.zw));

    projected_aabb *= VIRTUAL_RESOLUTION;

    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    StructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];

    overlap_page_bounds.xy = max(overlap_page_bounds.xy, required_page_bounds.xy);
    overlap_page_bounds.zw = min(overlap_page_bounds.zw, required_page_bounds.zw);

    if (overlap_page_bounds.x > overlap_page_bounds.z || overlap_page_bounds.y > overlap_page_bounds.w)
    {
        return false;
    }

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];
    float depth = (sphere_vs.z - sphere_vs.w) * vsm.matrix_p[2][2] + vsm.matrix_p[2][3];

    for (uint x = overlap_page_bounds.x; x <= overlap_page_bounds.z; ++x)
    {
        for (uint y = overlap_page_bounds.y; y <= overlap_page_bounds.w; ++y)
        {
            uint virtual_page_index = get_virtual_page_index(vsm_id, uint2(x, y));
            vsm_virtual_page virtual_page = unpack_virtual_page(virtual_page_table[virtual_page_index]);

            if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) == 0 || (virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID) != 0)
            {
                continue;
            }

            uint physical_page_index = get_physical_page_index(virtual_page.physical_page_coord);
            vsm_physical_page physical_page = unpack_physical_page(physical_page_table[physical_page_index]);

            if (physical_page.frame == 0)
            {
                return true;
            }

            float2 virtual_texel_offset = float2(x, y) * PAGE_RESOLUTION;

            float4 page_aabb;
            page_aabb.xy = max(virtual_texel_offset, projected_aabb.xy);
            page_aabb.zw = min(virtual_texel_offset + PAGE_RESOLUTION, projected_aabb.zw);

            float width = abs(page_aabb.z - page_aabb.x);
            float height = abs(page_aabb.w - page_aabb.y);

            if (width == 0 || height == 0)
            {
                continue;
            }

            float2 physical_texel_offset = virtual_page.physical_page_coord * PAGE_RESOLUTION;

            page_aabb.xy = page_aabb.xy - virtual_texel_offset + physical_texel_offset;
            page_aabb.zw = page_aabb.zw - virtual_texel_offset + physical_texel_offset;

            float level = clamp(floor(log2(max(width, height))), 0.0, 5.0);
            bool visiable = hzb.SampleLevel(hzb_sampler, (page_aabb.xy + page_aabb.zw) * 0.5 / PHYSICAL_RESOLUTION, level) < depth;

            if (visiable)
            {
                return true;
            }
        }
    }

    return false;
}

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    uint instance_id = dtid.x;
    if (instance_id >= scene.instance_count)
    {
        return;
    }

    StructuredBuffer<uint> visible_light_count = ResourceDescriptorHeap[constant.visible_light_count];
    uint vsm_count = visible_light_count[1];
    
    StructuredBuffer<uint> vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];
    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    StructuredBuffer<uint4> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];

    RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];
    RWStructuredBuffer<draw_command> draw_commands = ResourceDescriptorHeap[constant.draw_buffer];
    RWStructuredBuffer<vsm_draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    instance_data instance = instances[instance_id];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    geometry_data geometry = geometries[instance.geometry_index];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh = meshes[instance.mesh_index];

    float sphere_vs_radius = geometry.bounding_sphere.w * mesh.scale.w;
    if (sphere_vs_radius <= 0.0)
    {
        return;
    }

    uint draw_queue_rear = 0;
    vsm_draw_info draw_queue[32];

    for (uint i = 0; i < vsm_count && draw_queue_rear < 32; ++i)
    {
        uint vsm_id = vsm_ids[i];
        vsm_data vsm = vsms[vsm_id];
        uint4 required_page_bounds = vsm_bounds[vsm_id];

        if (required_page_bounds.x > required_page_bounds.z || required_page_bounds.y > required_page_bounds.w)
        {
            continue;
        }

        float4 sphere_vs = mul(vsm.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
        sphere_vs.w = sphere_vs_radius;

        if (overlap_required_page(vsm_id, required_page_bounds, sphere_vs, vsm))
        {
            vsm_draw_info draw_info;
            draw_info.vsm_id = vsm_id;
            draw_info.instance_id = instance_id;
            draw_queue[draw_queue_rear] = draw_info;

            ++draw_queue_rear;
        }
    }

    uint draw_command_offset = 0;
    InterlockedAdd(draw_counts[0], draw_queue_rear, draw_command_offset);

    if (draw_command_offset + draw_queue_rear > MAX_SHADOW_DRAWS_PER_FRAME)
    {
        return;
    }

    if (geometry.cluster_root != 0xFFFFFFFF)
    {
        RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
        RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

        uint cluster_node_queue_rear = 0;
        InterlockedAdd(cluster_queue_state[0].cluster_node_queue_rear, draw_queue_rear, cluster_node_queue_rear);

        for (uint i = 0; i < draw_queue_rear; ++i)
        {
            uint2 cluster_queue_item;
            cluster_queue_item.x = instance_id;
            cluster_queue_item.y = draw_queue[i].vsm_id << 24 | geometry.cluster_root;
            cluster_queue[cluster_node_queue_rear + i] = cluster_queue_item;
        }
    }
    else
    {
        for (uint i = 0; i < draw_queue_rear; ++i)
        {
            uint command_index = draw_command_offset + i;

            draw_command command;
            command.index_count = geometry.index_count;
            command.instance_count = 1;
            command.index_offset = geometry.index_offset;
            command.vertex_offset = 0;
            command.instance_offset = command_index;
            draw_commands[command_index] = command;

            draw_infos[command_index] = draw_queue[i];
        }
    }
}