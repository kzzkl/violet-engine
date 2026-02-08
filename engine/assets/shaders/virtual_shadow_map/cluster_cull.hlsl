#include "cluster.hlsli"
#include "virtual_shadow_map/vsm_cull.hlsli"

struct constant_data
{
    uint vsm_buffer;
    uint vsm_bounds_buffer;
    uint vsm_virtual_page_table;
    uint vsm_physical_page_table;
    
    uint hzb;
    uint hzb_sampler;

    float threshold;

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

ConstantBuffer<scene_data> scene : register(b0, space1);

groupshared uint gs_queue_offset;
groupshared uint gs_cluster_node_mask;
groupshared uint3 gs_cluster_node[MAX_CLUSTER_NODE_PER_GROUP]; // x: child_offset, y: child_count, z: instance_id
groupshared uint gs_vsms[MAX_CLUSTER_NODE_PER_GROUP];

struct cluster_node_item
{
    uint vsm_id;
    uint cluster_node_id;
    uint instance_id;

    static cluster_node_item unpack(uint2 packed_data)
    {
        cluster_node_item item;
        item.vsm_id = packed_data.x >> 24;
        item.cluster_node_id = packed_data.x & 0xFFFFFF;
        item.instance_id = packed_data.y;
        return item;
    }

    uint2 pack()
    {
        return uint2(vsm_id << 24 | cluster_node_id, instance_id);
    }
};

struct cluster_item
{
    uint vsm_id;
    uint cluster_id;
    uint instance_id;

    static cluster_item unpack(uint2 packed_data)
    {
        cluster_item item;
        item.vsm_id = packed_data.x >> 24;
        item.cluster_id = packed_data.x & 0xFFFFFF;
        item.instance_id = packed_data.y;
        return item;
    }

    uint2 pack()
    {
        return uint2(vsm_id << 24 | cluster_id, instance_id);
    }
};

uint get_cluster_node_offset()
{
    return 0;
}

uint get_cluster_offset()
{
    return constant.max_cluster_node_count;
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
    cluster_node_data cluster_node = cluster_nodes[cluster_node_id];

    instance_data instance = instances[instance_id];
    mesh_data mesh = meshes[instance.mesh_index];

    bool is_static = (mesh.flags & MESH_STATIC) != 0;

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    StructuredBuffer<vsm_bounds> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];

    uint vsm_id = gs_vsms[parent_index];
    vsm_data vsm = vsms[vsm_id];
    uint4 page_bounds = is_static ? vsm_bounds[vsm_id].invalidated_bounds : vsm_bounds[vsm_id].required_bounds;

    float4 sphere_vs = mul(vsm.matrix_v, mul(mesh.matrix_m, float4(cluster_node.bounding_sphere.xyz, 1.0)));
    sphere_vs.w = cluster_node.bounding_sphere.w * mesh.scale.w;

    camera_data camera;
    camera.type = CAMERA_ORTHOGRAPHIC;
    camera.matrix_v = vsm.matrix_v;
    camera.near = -vsm.view_z_radius;
    camera.pixels_per_unit = vsm.pixels_per_unit;
    bool visible = cluster_node.check_lod(camera, mesh, constant.threshold);

    if (visible)
    {
#ifdef USE_OCCLUSION
        StructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];
        Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
        SamplerState hzb_sampler = ResourceDescriptorHeap[constant.hzb_sampler];
        if (!vsm_cull(vsm_id, page_bounds, sphere_vs, is_static, vsm, virtual_page_table, physical_page_table, hzb, hzb_sampler))
#else
        if (!vsm_cull(vsm_id, page_bounds, sphere_vs, is_static, vsm, virtual_page_table))
#endif
        {
            visible = false;
        }
    }

    if (visible)
    {
        RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];

        if (cluster_node.is_leaf == 0)
        {
            uint cluster_node_queue_rear = 0;
            InterlockedAdd(cluster_queue_state[0].cluster_node_queue_rear, 1, cluster_node_queue_rear);

            cluster_node_item item;
            item.vsm_id = vsm_id;
            item.cluster_node_id = cluster_node_id;
            item.instance_id = instance_id;
            cluster_queue[cluster_node_queue_rear] = item.pack();
        }
        else
        {
            uint cluster_queue_rear = 0;
            InterlockedAdd(cluster_queue_state[0].cluster_queue_rear, cluster_node.child_count, cluster_queue_rear);
            cluster_queue_rear += get_cluster_offset();

            for (uint i = 0; i < cluster_node.child_count; ++i)
            {
                cluster_item item;
                item.vsm_id = vsm_id;
                item.cluster_id = cluster_node.child_offset + i;
                item.instance_id = instance_id;
                cluster_queue[cluster_queue_rear + i] = item.pack();
            }
        }
    }
}

void process_cluster(uint3 dtid)
{
    RWStructuredBuffer<uint2> cluster_queue = ResourceDescriptorHeap[constant.cluster_queue];
    StructuredBuffer<cluster_data> clusters = ResourceDescriptorHeap[scene.cluster_buffer];

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];

    cluster_item item = cluster_item::unpack(cluster_queue[dtid.x + get_cluster_offset()]);

    instance_data instance = instances[item.instance_id];
    mesh_data mesh = meshes[instance.mesh_index];
    geometry_data geometry = geometries[instance.geometry_index];

    cluster_data cluster = clusters[item.cluster_id];
    if (cluster.index_count == 0)
    {
        return;
    }

    bool is_static = (mesh.flags & MESH_STATIC) != 0;

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    StructuredBuffer<vsm_bounds> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];

    vsm_data vsm = vsms[item.vsm_id];
    uint4 page_bounds = is_static ? vsm_bounds[item.vsm_id].invalidated_bounds : vsm_bounds[item.vsm_id].required_bounds;

    float4 sphere_vs = mul(vsm.matrix_v, mul(mesh.matrix_m, float4(cluster.bounding_sphere.xyz, 1.0)));
    sphere_vs.w = cluster.bounding_sphere.w * mesh.scale.w;

    camera_data camera;
    camera.type = CAMERA_ORTHOGRAPHIC;
    camera.matrix_v = vsm.matrix_v;
    camera.near = -vsm.view_z_radius;
    camera.pixels_per_unit = vsm.pixels_per_unit;
    bool visible = cluster.check_lod(camera, mesh, constant.threshold);

    if (visible)
    {
#ifdef USE_OCCLUSION
        StructuredBuffer<uint4> physical_page_table = ResourceDescriptorHeap[constant.vsm_physical_page_table];
        Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
        SamplerState hzb_sampler = ResourceDescriptorHeap[constant.hzb_sampler];
        if (!vsm_cull(item.vsm_id, page_bounds, sphere_vs, is_static, vsm, virtual_page_table, physical_page_table, hzb, hzb_sampler))
#else
        if (!vsm_cull(item.vsm_id, page_bounds, sphere_vs, is_static, vsm, virtual_page_table))
#endif
        {
            visible = false;
        }
    }

    if (visible)
    {
        RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];

        uint command_index = 0;
        InterlockedAdd(draw_counts[is_static ? 0 : 1], 1, command_index);

        command_index += is_static ? STATIC_INSTANCE_DRAW_OFFSET : DYNAMIC_INSTANCE_DRAW_OFFSET;

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

            vsm_draw_info draw_info;
            draw_info.vsm_id = item.vsm_id;
            draw_info.instance_id = item.instance_id;
            draw_info.cluster_id = item.cluster_id;

            RWStructuredBuffer<vsm_draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];
            draw_infos[command_index] = draw_info;
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

    if (ready)
    {
        StructuredBuffer<cluster_node_data> cluster_nodes = ResourceDescriptorHeap[constant.cluster_node_buffer];

        cluster_node_item item = cluster_node_item::unpack(cluster_queue[queue_index]);

        cluster_node_data cluster_node = cluster_nodes[item.cluster_node_id];
        gs_cluster_node[group_index] = uint3(cluster_node.child_offset, cluster_node.child_count, item.instance_id);
        gs_vsms[group_index] = item.vsm_id;

        InterlockedOr(gs_cluster_node_mask, 1u << group_index);
    }
    GroupMemoryBarrierWithGroupSync();

    if (gs_cluster_node_mask != 0)
    {
        process_cluster_node(group_index);
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