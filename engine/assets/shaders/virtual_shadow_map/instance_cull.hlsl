#include "common.hlsli"
#include "cluster.hlsli"
#include "virtual_shadow_map/vsm_cull.hlsli"

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

        StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
#ifdef USE_OCCLUSION
        StructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];
        Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
        SamplerState hzb_sampler = ResourceDescriptorHeap[constant.hzb_sampler];
        if (vsm_cull(vsm_id, required_page_bounds, sphere_vs, vsm, virtual_page_table, physical_page_table, hzb, hzb_sampler))
#else
        if (vsm_cull(vsm_id, required_page_bounds, sphere_vs, vsm, virtual_page_table))
#endif
        {
            vsm_draw_info draw_info;
            draw_info.vsm_id = vsm_id;
            draw_info.instance_id = instance_id;
            draw_queue[draw_queue_rear] = draw_info;

            ++draw_queue_rear;
        }
    }

    RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];
    RWStructuredBuffer<draw_command> draw_commands = ResourceDescriptorHeap[constant.draw_buffer];
    RWStructuredBuffer<vsm_draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];

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
            cluster_queue_item.x = draw_queue[i].vsm_id << 24 | geometry.cluster_root;
            cluster_queue_item.y = instance_id;
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