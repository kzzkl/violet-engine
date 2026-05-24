#include "cull/cull.hlsli"
#include "cluster.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct constant_data
{
    uint hzb;
    uint hzb_sampler;
    uint draw_buffer;
    uint draw_count_buffer;
    uint draw_info_buffer;
    uint cluster_queue;
    uint cluster_queue_state;
    uint max_draw_command_count;
    uint recheck_instances;
    uint recheck_count;
};
PushConstant(constant_data, constant);

groupshared uint gs_recheck_offset;
groupshared uint gs_recheck_count;

groupshared uint gs_visible_cluster_offset;
groupshared uint gs_visible_cluster_count;

#ifndef CULL_MAIN_PASS
#define CULL_MAIN_PASS 0
#endif

uint get_instance_id(uint3 dtid)
{
#if CULL_MAIN_PASS
    if (dtid.x < scene.instance_count)
    {
        return dtid.x;
    }
#else
    StructuredBuffer<uint> recheck_instances = ResourceDescriptorHeap[constant.recheck_instances];
    StructuredBuffer<uint> recheck_count = ResourceDescriptorHeap[constant.recheck_count];

    if (dtid.x < recheck_count[0])
    {
        return recheck_instances[dtid.x];
    }
#endif

    return 0xFFFFFFFF;
}

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    instance_data instance;

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    geometry_data geometry;

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh;

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

    uint instance_id = get_instance_id(dtid);
    if (instance_id != 0xFFFFFFFF)
    {
        instance = instances[instance_id];
        geometry = geometries[instance.geometry_index];
        mesh = meshes[instance.mesh_index];
    }

    bool visible = instance_id != 0xFFFFFFFF;

    if (group_index == 0)
    {
        gs_recheck_count = 0;
        gs_visible_cluster_count = 0;
    }

#if CULL_MAIN_PASS
    GroupMemoryBarrierWithGroupSync();

    if (visible && (mesh.flags & MESH_SKIP_FRUSTUM_CULL) == 0)
    {
        float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
        sphere_vs.w = geometry.bounding_sphere.w * mesh.scale.w;
        visible = sphere_vs.w > 0.0;
        visible = visible && frustum_cull(sphere_vs, camera);
    }

    uint recheck_offset = 0xFFFFFFFF;
    if (visible && (mesh.flags & MESH_SKIP_OCCLUSION_CULL) == 0)
    {
        float4 prev_sphere_vs = mul(camera.prev_matrix_v, mul(mesh.prev_matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
        prev_sphere_vs.w = geometry.bounding_sphere.w * mesh.scale.w;
        if (!occlusion_cull(
                prev_sphere_vs,
                hzb,
                hzb_sampler,
                camera.prev_matrix_p,
                camera.near,
                camera.type))
        {
            visible = false;
            InterlockedAdd(gs_recheck_count, 1, recheck_offset);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        RWStructuredBuffer<uint> recheck_count = ResourceDescriptorHeap[constant.recheck_count];
        InterlockedAdd(recheck_count[0], gs_recheck_count, gs_recheck_offset);
    }
    
    GroupMemoryBarrierWithGroupSync();

    if (recheck_offset != 0xFFFFFFFF)
    {
        RWStructuredBuffer<uint> recheck_instances = ResourceDescriptorHeap[constant.recheck_instances];
        recheck_instances[gs_recheck_offset + recheck_offset] = instance_id;
    }
#else
    if (visible)
    {
        float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
        sphere_vs.w = geometry.bounding_sphere.w * mesh.scale.w;
        visible = occlusion_cull(
            sphere_vs,
            hzb,
            hzb_sampler,
            camera.matrix_p,
            camera.near,
            camera.type);
    }
#endif

#ifdef GENERATE_CLUSTER_LIST
    uint visible_cluster_offset = 0xFFFFFFFF;
    if (visible && geometry.cluster_root != 0xFFFFFFFF)
    {
        InterlockedAdd(gs_visible_cluster_count, 1, visible_cluster_offset);
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0 && gs_visible_cluster_count != 0)
    {
        RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];
        InterlockedAdd(cluster_queue_state[0].cluster_node_queue_rear, gs_visible_cluster_count, gs_visible_cluster_offset);
    }

    GroupMemoryBarrierWithGroupSync();
#endif

    if (!visible)
    {
        return;
    }

#ifdef GENERATE_CLUSTER_LIST
    if (geometry.cluster_root != 0xFFFFFFFF)
    {
        RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
        cluster_queue[gs_visible_cluster_offset + visible_cluster_offset] = uint2(geometry.cluster_root, instance_id);
    }
    else
    {
#endif
        StructuredBuffer<uint> batch_buffer = ResourceDescriptorHeap[scene.batch_buffer];
        RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];

        uint batch_index = instance.batch_index;

        uint command_index = 0;
        InterlockedAdd(draw_counts[batch_index], 1, command_index);
        command_index += batch_buffer[batch_index];

        if (command_index < constant.max_draw_command_count)
        {
            draw_command command;
            command.index_count = geometry.index_count;
            command.instance_count = 1;
            command.index_offset = geometry.index_offset;
            command.vertex_offset = 0;
            command.instance_offset = command_index;

            RWStructuredBuffer<draw_command> draw_commands = ResourceDescriptorHeap[constant.draw_buffer];
            draw_commands[command_index] = command;

            draw_info info;
            info.instance_id = instance_id;
            info.cluster_id = 0xFFFFFFFF;

            RWStructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
            draw_infos[command_index] = info;
        }

#ifdef GENERATE_CLUSTER_LIST
    }
#endif
}