#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint shadow_light_buffer;
    uint shadow_light_count;
    uint vsm_info;
    uint visible_light_list;
    uint visible_vsm_list;
    uint virtual_pages_indirect_args;
    uint camera_id;
    uint vsm_directional_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= constant.shadow_light_count)
    {
        return;
    }

    StructuredBuffer<uint2> shadow_lights = ResourceDescriptorHeap[constant.shadow_light_buffer];
    uint light_id = shadow_lights[dtid.x].x;
    uint vsm_address = shadow_lights[dtid.x].y;

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];
    light_data light = lights[light_id];

    RWStructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
    RWStructuredBuffer<uint> visible_light_list = ResourceDescriptorHeap[constant.visible_light_list];
    RWStructuredBuffer<uint> visible_vsm_list = ResourceDescriptorHeap[constant.visible_vsm_list];

    RWStructuredBuffer<dispatch_command> virtual_pages_indirect_args = ResourceDescriptorHeap[constant.virtual_pages_indirect_args];

    if (light.type == LIGHT_DIRECTIONAL)
    {
        StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];

        uint vsm_id = get_directional_vsm_id(directional_vsms, vsm_address, constant.camera_id);

        uint light_index = 0;
        InterlockedAdd(vsm_info[0].visible_light_count, 1, light_index);
        visible_light_list[light_index] = dtid.x;

        uint vsm_index = 0;
        InterlockedAdd(vsm_info[0].visible_vsm_count, 16, vsm_index);

        for (uint i = 0; i < 16; ++i)
        {
            visible_vsm_list[vsm_index + i] = vsm_id + i;
        }

        InterlockedAdd(virtual_pages_indirect_args[0].z, 16);
    }
}