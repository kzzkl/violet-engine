#include "common.hlsli"
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

    float threshold;
    float lod_scale;

    uint cluster_node_buffer;
    uint cluster_queue;
    uint cluster_queue_state;

    uint draw_buffer;
    uint draw_count_buffer;
    uint draw_info_buffer;

    uint max_cluster_count;
    uint max_cluster_node_count;
    uint max_draw_command_count;
};
PushConstant(constant_data, constant);

groupshared uint gs_queue_offset;
groupshared uint gs_cluster_node_mask;
groupshared uint3 gs_cluster_node[MAX_CLUSTER_NODE_PER_GROUP]; // x: child_offset, y: child_count, z: instance_id
groupshared uint gs_recheck_mask[MAX_CLUSTER_NODE_PER_GROUP];

uint2 pack_cluster_node_item(uint id, uint instance_id, uint recheck_mask)
{
    return uint2(recheck_mask << 24 | id, instance_id);
}

void unpack_cluster_node_item(uint2 item, out uint id, out uint instance_id, out uint recheck_mask)
{
    id = item.x & 0xFFFFFF;
    instance_id = item.y;
    recheck_mask = item.x >> 24;
}

uint2 pack_cluster_item(uint id, uint instance_id, bool recheck)
{
    return uint2((recheck ? 1u : 0u) << 31 | id, instance_id);
}

void unpack_cluster_item(uint2 item, out uint id, out uint instance_id, out bool recheck)
{
    id = item.x & 0x7FFFFFFF;
    instance_id = item.y;
    recheck = (item.x >> 31) == 1;
}

uint get_cluster_node_offset()
{
    return 0;
}

uint get_cluster_offset()
{
    return constant.max_cluster_node_count;
}

bool check_cluster_node_lod(cluster_node_data cluster_node, mesh_data mesh)
{
    float3 center = mul(camera.matrix_v, mul(mesh.matrix_m, float4(cluster_node.lod_bounds.xyz, 1.0))).xyz;
    float radius = cluster_node.lod_bounds.w * mesh.scale.w;

    float near = center.z - radius;
    float far = center.z + radius;

    if (near < camera.near)
    {
        return true;
    }

    float scale = constant.lod_scale * mesh.scale.w;

    float min_error = scale * cluster_node.min_lod_error / far;
    float max_error = scale * cluster_node.max_parent_lod_error / near;

    return min_error <= constant.threshold && constant.threshold < max_error;
}

bool check_cluster_lod(cluster_data cluster, mesh_data mesh)
{
    float3 center = mul(camera.matrix_v, mul(mesh.matrix_m, float4(cluster.lod_bounds.xyz, 1.0))).xyz;
    float radius = cluster.lod_bounds.w * mesh.scale.w;

    float near = center.z - radius;
    
    // if camera inside lod sphere, use lod 0.
    if (near < camera.near)
    {
        return cluster.lod_error == -1.0;
    }

    float lod_error = constant.lod_scale * cluster.lod_error * mesh.scale.w / near;
    return lod_error <= constant.threshold;
}

void process_cluster_node(uint group_index)
{
    StructuredBuffer<cluster_node_data> cluster_nodes = ResourceDescriptorHeap[constant.cluster_node_buffer];
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];

    uint parent_index = group_index / MAX_CLUSTER_NODE_PER_GROUP;

    if ((gs_cluster_node_mask & (1u << parent_index)) == 0)
    {
        return;
    }

    uint3 item = gs_cluster_node[parent_index];
    uint child_offset = item.x;
    uint child_count = item.y;
    uint instance_id = item.z;

    uint child_index = group_index % MAX_CLUSTER_NODE_PER_GROUP;
    if (child_index >= child_count)
    {
        return;
    }

    uint cluster_node_id = child_index + child_offset;

    instance_data instance = instances[instance_id];
    mesh_data mesh = meshes[instance.mesh_index];

    cluster_node_data cluster_node = cluster_nodes[cluster_node_id];

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

    float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(cluster_node.bounding_sphere.xyz, 1.0)));
    sphere_vs.w = cluster_node.bounding_sphere.w * mesh.scale.w;

    bool visible = true;

#if CULL_STAGE == CULL_STAGE_MAIN_PASS
    visible = check_cluster_node_lod(cluster_node, mesh) && frustum_cull(sphere_vs, constant.frustum, camera.near);

    if (visible)
    {
        float4 prev_sphere_vs = mul(camera.prev_matrix_v, mul(mesh.prev_matrix_m, float4(cluster_node.bounding_sphere.xyz, 1.0)));
        prev_sphere_vs.w = sphere_vs.w;
        if (!occlusion_cull(prev_sphere_vs, hzb, hzb_sampler, constant.hzb_width, constant.hzb_height, camera.prev_matrix_p, camera.near))
        {
            visible = false;
            InterlockedOr(gs_recheck_mask[parent_index], 1u << child_index);
        }
    }
#else
    if (gs_queue_offset + group_index < cluster_queue_state[0].cluster_node_recheck_size)
    {
        visible = gs_recheck_mask[parent_index] & (1u << child_index);
    }
    else
    {
        visible = check_cluster_node_lod(cluster_node, mesh) && frustum_cull(sphere_vs, constant.frustum, camera.near);
    }

    visible = visible && occlusion_cull(sphere_vs, hzb, hzb_sampler, constant.hzb_width, constant.hzb_height, camera.matrix_p, camera.near);
#endif

    if (visible)
    {
        RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];

        if (cluster_node.is_leaf == 0)
        {
            uint cluster_node_queue_rear = 0;
            InterlockedAdd(cluster_queue_state[0].cluster_node_queue_rear, 1, cluster_node_queue_rear);
            cluster_queue[cluster_node_queue_rear] = pack_cluster_node_item(cluster_node_id, instance_id, 0);
        }
        else
        {
            uint cluster_queue_rear = 0;
            InterlockedAdd(cluster_queue_state[0].cluster_queue_rear, cluster_node.child_count, cluster_queue_rear);
            cluster_queue_rear += get_cluster_offset();

            for (uint i = 0; i < cluster_node.child_count; ++i)
            {
                cluster_queue[cluster_queue_rear + i] = pack_cluster_item(cluster_node.child_offset + i, instance_id, false);
            }
        }
    }
}

void process_cluster(uint3 dtid)
{
    RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];
    StructuredBuffer<cluster_data> clusters = ResourceDescriptorHeap[scene.cluster_buffer];

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];

    uint cluster_id;
    uint instance_id;
    bool recheck;
    unpack_cluster_item(cluster_queue[dtid.x + get_cluster_offset()], cluster_id, instance_id, recheck);

    instance_data instance = instances[instance_id];
    mesh_data mesh = meshes[instance.mesh_index];
    geometry_data geometry = geometries[instance.geometry_index];

    cluster_data cluster = clusters[cluster_id];
    if (cluster.index_count == 0)
    {
        return;
    }

    bool visible = true;

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];
    
    float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(cluster.bounding_sphere.xyz, 1.0)));
    sphere_vs.w = cluster.bounding_sphere.w * mesh.scale.w;

#if CULL_STAGE == CULL_STAGE_MAIN_PASS
    visible = check_cluster_lod(cluster, mesh) && frustum_cull(sphere_vs, constant.frustum, camera.near);

    if (visible)
    {
        float4 prev_sphere_vs = mul(camera.prev_matrix_v, mul(mesh.prev_matrix_m, float4(cluster.bounding_sphere.xyz, 1.0)));
        prev_sphere_vs.w = sphere_vs.w;
        if (!occlusion_cull(prev_sphere_vs, hzb, hzb_sampler, constant.hzb_width, constant.hzb_height, camera.prev_matrix_p, camera.near))
        {
            visible = false;
            cluster_queue[dtid.x + get_cluster_offset()] = pack_cluster_item(cluster_id, instance_id, true);
        }
    }
#else
    if (dtid.x < cluster_queue_state[0].cluster_recheck_size)
    {
        visible = recheck;
    }
    else
    {
        visible = check_cluster_lod(cluster, mesh) && frustum_cull(sphere_vs, constant.frustum, camera.near);
    }

    visible = visible && occlusion_cull(sphere_vs, hzb, hzb_sampler, constant.hzb_width, constant.hzb_height, camera.matrix_p, camera.near);
#endif

    if (visible)
    {
        StructuredBuffer<uint> batch_buffer = ResourceDescriptorHeap[scene.batch_buffer];
        RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];

        uint batch_index = instance.batch_index;
        uint command_index = 0;
        InterlockedAdd(draw_counts[batch_index], 1, command_index);
        command_index += batch_buffer[batch_index];

        if (command_index < constant.max_draw_command_count)
        {
            draw_command command;
            command.index_count = cluster.index_count;
            command.instance_count = 1;
            command.index_offset = geometry.index_offset + cluster.index_offset;
            command.vertex_offset = 0;
            command.instance_offset = command_index;

            RWStructuredBuffer<draw_command> draw_commands = ResourceDescriptorHeap[constant.draw_buffer];
            draw_commands[command_index] = command;

            draw_info info;
            info.instance_id = instance_id;
            info.cluster_id = cluster_id;

            RWStructuredBuffer<draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
            draw_infos[command_index] = info;
        }
    }
}

void cluster_node_cull(uint group_index)
{
    RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

    if (group_index == 0)
    {
        gs_cluster_node_mask = 0;
        InterlockedAdd(cluster_queue_state[0].cluster_node_queue_front, MAX_CLUSTER_NODE_PER_GROUP, gs_queue_offset);
    }
    GroupMemoryBarrierWithGroupSync();
    
    uint queue_index = gs_queue_offset + group_index;

    bool ready = group_index < MAX_CLUSTER_NODE_PER_GROUP && queue_index < cluster_queue_state[0].cluster_node_queue_prev_rear;

    uint cluster_id;
    uint instance_id;

    if (ready)
    {
        StructuredBuffer<cluster_node_data> cluster_nodes = ResourceDescriptorHeap[constant.cluster_node_buffer];

        uint recheck_mask;
        unpack_cluster_node_item(cluster_queue[queue_index], cluster_id, instance_id, recheck_mask);

        cluster_node_data cluster_node = cluster_nodes[cluster_id];
        gs_cluster_node[group_index] = uint3(cluster_node.child_offset, cluster_node.child_count, instance_id);

        InterlockedOr(gs_cluster_node_mask, 1u << group_index);

#if CULL_STAGE == CULL_STAGE_MAIN_PASS
        gs_recheck_mask[group_index] = 0;
#else
        gs_recheck_mask[group_index] = recheck_mask;
#endif
    }
    GroupMemoryBarrierWithGroupSync();

    if (gs_cluster_node_mask != 0)
    {
        process_cluster_node(group_index);
    }

    GroupMemoryBarrierWithGroupSync();

    if (ready)
    {
        cluster_queue[queue_index] = pack_cluster_node_item(cluster_id, instance_id, gs_recheck_mask[group_index]);
    }
}

void cluster_cull(uint3 dtid, uint group_index)
{
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

    if (dtid.x < cluster_queue_state[0].cluster_queue_rear)
    {
        process_cluster(dtid);
    }
}

[numthreads(CLUSTER_CULL_GROUP_SIZE, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
#if defined(CULL_CLUSTER_NODE)
    cluster_node_cull(group_index);
#elif defined(CULL_CLUSTER)
    cluster_cull(dtid, group_index);
#endif
}