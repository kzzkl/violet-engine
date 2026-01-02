#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_light_buffer;
    uint page_table;
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

    StructuredBuffer<uint> visible_light_buffer = ResourceDescriptorHeap[constant.visible_light_buffer];
    StructuredBuffer<float4x4> vsm_matrices = ResourceDescriptorHeap[0];

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

    uint draw_queue_rear = 0;
    vsm_draw_info draw_queue[32];

    for (uint i = 0; i < 1; ++i)
    {
        uint vsm_id = get_visiable_vsm_id(visible_light_buffer, i);
        float4x4 vsm_matrix_vp = vsm_matrices[i];

        vsm_draw_info draw_info;
        draw_info.vsm_id = vsm_id;
        draw_info.instance_id = instance_id;
        draw_queue[draw_queue_rear] = draw_info;

        ++draw_queue_rear;
    }

    uint vsm_draw_queue_offset = 0;
    InterlockedAdd(draw_counts[0], draw_queue_rear, vsm_draw_queue_offset);

    for (uint i = 0; i < draw_queue_rear; ++i)
    {
        uint command_index = vsm_draw_queue_offset + i;

        draw_command command;
        command.index_count = geometry.index_count;
        command.instance_count = 1;
        command.index_offset = geometry.index_offset;
        command.vertex_offset = 0;
        command.instance_offset = command_index;
        draw_commands[vsm_draw_queue_offset + i] = command;

        draw_infos[command_index] = draw_queue[i];
    }
}