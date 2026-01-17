#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_light_count;
    uint visible_vsm_ids;
    uint vsm_buffer;
    uint vsm_projection_buffer;
    uint draw_buffer;
    uint draw_count_buffer;
    uint draw_info_buffer;
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
    StructuredBuffer<vsm_projection> vsm_projections = ResourceDescriptorHeap[constant.vsm_projection_buffer];

    RWStructuredBuffer<uint> draw_counts = ResourceDescriptorHeap[constant.draw_count_buffer];
    RWStructuredBuffer<draw_command> draw_commands = ResourceDescriptorHeap[constant.draw_buffer];
    RWStructuredBuffer<vsm_draw_info> draw_infos = ResourceDescriptorHeap[constant.draw_info_buffer];

    StructuredBuffer<instance_data> instances = ResourceDescriptorHeap[scene.instance_buffer];
    instance_data instance = instances[instance_id];

    StructuredBuffer<geometry_data> geometries = ResourceDescriptorHeap[scene.geometry_buffer];
    geometry_data geometry = geometries[instance.geometry_index];

    StructuredBuffer<mesh_data> meshes = ResourceDescriptorHeap[scene.mesh_buffer];
    mesh_data mesh = meshes[instance.mesh_index];

    // float4 sphere_vs = mul(camera.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
    // sphere_vs.w = geometry.bounding_sphere.w * mesh.scale.w;
    // visible = frustum_cull(sphere_vs, constant.frustum, camera.near);

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
        vsm_projection projection = vsm_projections[vsm_id];

        bool visible = true;

        float4 sphere_vs = mul(vsm.matrix_v, mul(mesh.matrix_m, float4(geometry.bounding_sphere.xyz, 1.0)));
        sphere_vs.w = sphere_vs_radius;

        // TODO: cull instance.

        if (visible)
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