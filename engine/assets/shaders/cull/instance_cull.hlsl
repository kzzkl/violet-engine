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
    uint command_buffer;
    uint count_buffer;
    uint cluster_queue;
    uint cluster_queue_state;
    uint max_draw_commands;
};
PushConstant(constant_data, constant);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    uint instance_index = dtid.x;
    if (instance_index >= scene.instance_count)
    {
        return;
    }

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    instance_data instance = instances[instance_index];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    geometry_data geometry = geometries[instance.geometry_index];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh = meshes[instance.mesh_index];

    float4 sphere_ws = mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0));
    sphere_ws.w = geometry.bounding_sphere.w * mesh.scale.w;

    bounding_sphere_cull cull = bounding_sphere_cull::create(sphere_ws, camera);

    Texture2D<float> hzb = ResourceDescriptorHeap[constant.hzb];
    SamplerState hzb_sampler = SamplerDescriptorHeap[constant.hzb_sampler];

    if (!cull.frustum_cull(constant.frustum) || !cull.occlusion_cull(hzb, hzb_sampler, constant.hzb_width, constant.hzb_height))
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
        cluster_queue[cluster_node_queue_rear] = uint2(geometry.cluster_root, instance_index);
    }
    else
    {
#endif
        ByteAddressBuffer batch_buffer = ResourceDescriptorHeap[scene.batch_buffer];
        RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.count_buffer];

        uint batch_index = instance.batch_index;

        uint command_index = 0;
        InterlockedAdd(draw_counts[batch_index], 1, command_index);
        command_index += batch_buffer.Load<uint>(batch_index * 4);

        if (command_index < constant.max_draw_commands)
        {
            draw_command command;
            command.index_count = geometry.index_count;
            command.instance_count = 1;
            command.index_offset = geometry.index_offset;
            command.vertex_offset = 0;
            command.instance_offset = instance_index;

            RWStructuredBuffer<draw_command> draw_commands = ResourceDescriptorHeap[constant.command_buffer];
            draw_commands[command_index] = command;
        }

#ifdef GENERATE_CLUSTER_LIST
    }
#endif
}