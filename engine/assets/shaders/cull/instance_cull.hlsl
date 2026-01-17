#include "cull/cull.hlsli"

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

struct constant_data
{
    uint hzb;
    uint hzb_sampler;
    uint hzb_width;
    uint hzb_height;
    float4 frustum;
    uint draw_buffer;
    uint draw_count_buffer;
    uint draw_info_buffer;
    uint cluster_queue;
    uint cluster_queue_state;
    uint max_draw_command_count;
    uint recheck_instances;
};
PushConstant(constant_data, constant);

groupshared uint gs_recheck_masks[2];

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    uint instance_id = dtid.x;
    if (instance_id >= scene.instance_count)
    {
        return;
    }

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    instance_data instance = instances[instance_id];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    geometry_data geometry = geometries[instance.geometry_index];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh = meshes[instance.mesh_index];

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

    bool visible = true;

#if CULL_STAGE == CULL_STAGE_MAIN_PASS
    uint recheck_mask_index = group_index / 32;

    if (group_index % 32 == 0)
    {
        gs_recheck_masks[recheck_mask_index] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
    sphere_vs.w = geometry.bounding_sphere.w * mesh.scale.w;

    visible = sphere_vs.w > 0.0 && frustum_cull(sphere_vs, constant.frustum, camera.near);

    if (visible)
    {
        float4 prev_sphere_vs = mul(camera.prev_matrix_v, mul(mesh.prev_matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
        prev_sphere_vs.w = sphere_vs.w;
        if (!occlusion_cull(prev_sphere_vs, hzb, hzb_sampler, constant.hzb_width, constant.hzb_height, camera.prev_matrix_p, camera.near))
        {
            visible = false;
            InterlockedOr(gs_recheck_masks[recheck_mask_index], 1u << (instance_id % 32));
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index % 32 == 0)
    {
        RWStructuredBuffer<uint> recheck_instances = ResourceDescriptorHeap[constant.recheck_instances];
        recheck_instances[instance_id / 32] = gs_recheck_masks[recheck_mask_index];
    }
#else
    StructuredBuffer<uint> recheck_instances = ResourceDescriptorHeap[constant.recheck_instances];
    uint recheck_mask = recheck_instances[instance_id / 32];

    visible = (recheck_mask & (1u << (instance_id % 32))) != 0;
    if (visible)
    {
        float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
        sphere_vs.w = geometry.bounding_sphere.w * mesh.scale.w;
        visible = occlusion_cull(sphere_vs, hzb, hzb_sampler, constant.hzb_width, constant.hzb_height, camera.matrix_p, camera.near);
    }
#endif

    if (!visible)
    {
        return;
    }

#ifdef GENERATE_CLUSTER_LIST
    if (geometry.cluster_root != 0xFFFFFFFF)
    {
        RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
        RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

        uint cluster_node_queue_rear = 0;
        InterlockedAdd(cluster_queue_state[0].cluster_node_queue_rear, 1, cluster_node_queue_rear);
        cluster_queue[cluster_node_queue_rear] = uint2(geometry.cluster_root, instance_id);
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