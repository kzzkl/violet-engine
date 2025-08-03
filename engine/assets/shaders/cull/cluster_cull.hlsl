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

    uint cluster_buffer;
    uint cluster_node_buffer;
    uint cluster_queue;
    uint cluster_queue_state;

    uint command_buffer;
    uint count_buffer;

    uint max_clusters;
    uint max_cluster_nodes;
    uint max_draw_commands;
};
PushConstant(constant_data, constant);

static const uint INVALID_NODE_INDEX = 0xFFFFFFFF;
static const uint INVALID_INSTANCE_INDEX = 0xFFFFFFFF;

groupshared uint group_queue_offset;
groupshared uint group_cluster_queue_rear;
groupshared uint group_cluster_node_mask;
groupshared uint3 group_cluster_node[MAX_NODE_PER_GROUP]; // x: child_offset, y: child_count, z: instance_index

uint get_cluster_node_offset()
{
    return 0;
}

uint get_cluster_offset()
{
    return constant.max_cluster_nodes;
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

bool load_group_cluster_node(uint group_index)
{
    RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
    StructuredBuffer<cluster_node_data> cluster_nodes = ResourceDescriptorHeap[constant.cluster_node_buffer];

    uint queue_index = group_queue_offset + group_index;

    uint2 item = cluster_queue[queue_index];
    cluster_queue[queue_index] = uint2(INVALID_NODE_INDEX, INVALID_INSTANCE_INDEX);

    if (item.x == INVALID_NODE_INDEX)
    {
        group_cluster_node[group_index] = 0;
        return false;
    }
    else
    {
        cluster_node_data cluster_node = cluster_nodes[item.x];
        group_cluster_node[group_index] = uint3(cluster_node.child_offset, cluster_node.child_count, item.y);
        return cluster_node.child_count != 0;
    }
}

void process_cluster_node(uint group_index)
{
    RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

    StructuredBuffer<cluster_node_data> cluster_nodes = ResourceDescriptorHeap[constant.cluster_node_buffer];

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];

    uint3 item = group_cluster_node[group_index / MAX_NODE_PER_GROUP];
    uint child_offset = item.x;
    uint child_count = item.y;
    uint instance_index = item.z;

    uint node_index = group_index % MAX_NODE_PER_GROUP;
    node_index = node_index < child_count ? node_index + child_offset : INVALID_NODE_INDEX;

    if (node_index != INVALID_NODE_INDEX)
    {
        instance_data instance = instances[instance_index];
        mesh_data mesh = meshes[instance.mesh_index];

        cluster_node_data cluster_node = cluster_nodes[node_index];

        bool visible = check_cluster_node_lod(cluster_node, mesh);

        if (visible)
        {
            Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
            SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

            float4 sphere_ws = mul(mesh.matrix_m, float4(cluster_node.bounding_sphere.xyz, 1.0));
            sphere_ws.w = cluster_node.bounding_sphere.w * mesh.scale.w;

            bounding_sphere_cull cull = bounding_sphere_cull::create(sphere_ws, camera);
            visible = cull.frustum_cull(constant.frustum) && cull.occlusion_cull(hzb, hzb_sampler, constant.hzb_width, constant.hzb_height);
        }

        if (visible)
        {
            if (cluster_node.is_leaf == 0)
            {
                uint cluster_node_queue_rear = 0;
                InterlockedAdd(cluster_queue_state[0].cluster_node_queue_rear, 1, cluster_node_queue_rear);
                cluster_queue[cluster_node_queue_rear] = uint2(node_index, instance_index);
            }
            else
            {
                uint cluster_queue_rear = 0;
                InterlockedAdd(cluster_queue_state[0].cluster_queue_rear, cluster_node.child_count, cluster_queue_rear);
                cluster_queue_rear += get_cluster_offset();

                for (uint i = 0; i < cluster_node.child_count; ++i)
                {
                    cluster_queue[cluster_queue_rear + i] = uint2(cluster_node.child_offset + i, instance_index);
                }
            }
        }
    }
}

void process_cluster(uint3 dtid)
{
    RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
    StructuredBuffer<cluster_data> clusters = ResourceDescriptorHeap[constant.cluster_buffer];

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];

    uint queue_index = dtid.x + get_cluster_offset();
    uint2 item = cluster_queue[queue_index];
    cluster_queue[queue_index] = uint2(INVALID_NODE_INDEX, INVALID_INSTANCE_INDEX);

    uint cluster_index = item.x;
    uint instance_index = item.y;

    instance_data instance = instances[instance_index];
    mesh_data mesh = meshes[instance.mesh_index];
    geometry_data geometry = geometries[instance.geometry_index];

    cluster_data cluster = clusters[cluster_index];
    if (cluster.index_count == 0)
    {
        return;
    }

    bool visible = check_cluster_lod(cluster, mesh);

    if (visible)
    {
        Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
        SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

        float4 sphere_ws = mul(mesh.matrix_m, float4(cluster.bounding_sphere.xyz, 1.0));
        sphere_ws.w = cluster.bounding_sphere.w * mesh.scale.w;

        bounding_sphere_cull cull = bounding_sphere_cull::create(sphere_ws, camera);
        visible = cull.frustum_cull(constant.frustum) && cull.occlusion_cull(hzb, hzb_sampler, constant.hzb_width, constant.hzb_height);
    }

    if (visible)
    {
        ByteAddressBuffer batch_buffer = ResourceDescriptorHeap[scene.batch_buffer];
        RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.count_buffer];

        uint batch_index = instance.batch_index;
        uint command_index = 0;
        InterlockedAdd(draw_counts[batch_index], 1, command_index);
        command_index += batch_buffer.Load<uint>(batch_index * 4);

        if (command_index < constant.max_draw_commands)
        {
            draw_command command;
            command.index_count = cluster.index_count;
            command.instance_count = 1;
            command.index_offset = geometry.index_offset + cluster.index_offset;
            command.vertex_offset = 0;
            command.instance_offset = instance_index;

            RWStructuredBuffer<draw_command> draw_commands = ResourceDescriptorHeap[constant.command_buffer];
            draw_commands[command_index] = command;
        }
    }
}

void cluster_node_cull(uint group_index)
{
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

    if (group_index == 0)
    {
        group_cluster_queue_rear = cluster_queue_state[0].cluster_node_queue_prev_rear;
        group_cluster_node_mask = 0;
        InterlockedAdd(cluster_queue_state[0].cluster_node_queue_front, MAX_NODE_PER_GROUP, group_queue_offset);
    }
    GroupMemoryBarrierWithGroupSync();

    bool ready = group_index < MAX_NODE_PER_GROUP;

    if (ready)
    {
        ready = load_group_cluster_node(group_index);
    }

    if (ready)
    {
        InterlockedOr(group_cluster_node_mask, 1u << group_index);
    }
    GroupMemoryBarrierWithGroupSync();

    if (group_cluster_node_mask != 0)
    {
        process_cluster_node(group_index);
    }
}

void cluster_cull(uint3 dtid, uint group_index)
{
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

    if (group_index == 0)
    {
        group_cluster_queue_rear = cluster_queue_state[0].cluster_queue_rear;
    }
    GroupMemoryBarrierWithGroupSync();

    bool ready = dtid.x < group_cluster_queue_rear;

    if (ready)
    {
        process_cluster(dtid);
    }
}

void persistent_cull(uint group_index)
{
    RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
    RWStructuredBuffer<cluster_queue_state_data> cluster_queue_state = ResourceDescriptorHeap[constant.cluster_queue_state];

    // while (true)
    for (uint i = 0; i < 2; ++i)
    {
        if (group_index == 0)
        {
            group_cluster_node_mask = 0;
            InterlockedAdd(cluster_queue_state[0].cluster_node_queue_front, MAX_NODE_PER_GROUP, group_queue_offset);
        }
        GroupMemoryBarrierWithGroupSync();

        bool ready = group_index < MAX_NODE_PER_GROUP;

        if (ready)
        {
            ready = load_group_cluster_node(group_index);
        }

        if (ready)
        {
            InterlockedOr(group_cluster_node_mask, 1u << group_index);
        }
        GroupMemoryBarrierWithGroupSync();

        if (group_cluster_node_mask != 0)
        {
            process_cluster_node(group_index);
            continue;
        }
    }
}

[numthreads(CLUSTER_CULL_GROUP_SIZE, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
#if defined(CULL_CLUSTER_NODE)
    cluster_node_cull(group_index);
#elif defined(CULL_CLUSTER)
    cluster_cull(dtid, group_index);
#else
    // persistent_cull(group_index);
#endif
}