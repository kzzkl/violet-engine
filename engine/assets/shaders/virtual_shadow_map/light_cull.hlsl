#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_info;
    uint visible_light_list;
    uint visible_vsm_list;
    uint clear_virtual_page_table_indirect_args;
    uint camera_id;
    uint vsm_directional_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= scene.shadow_casting_light_count)
    {
        return;
    }

    uint light_id = dtid.x;

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.shadow_casting_light_buffer];
    light_data light = lights[light_id];

    if (light.vsm_address == 0xFFFFFFFF)
    {
        return;
    }

    RWStructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
    RWStructuredBuffer<uint> visible_light_list = ResourceDescriptorHeap[constant.visible_light_list];
    RWStructuredBuffer<uint> visible_vsm_list = ResourceDescriptorHeap[constant.visible_vsm_list];

    RWStructuredBuffer<dispatch_command> clear_virtual_page_table_indirect_args = ResourceDescriptorHeap[constant.clear_virtual_page_table_indirect_args];

    if (light.type == LIGHT_DIRECTIONAL)
    {
        StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.vsm_directional_buffer];

        uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, constant.camera_id);

        uint light_index = 0;
        InterlockedAdd(vsm_info[0].visible_light_count, 1, light_index);
        visible_light_list[light_index] = dtid.x;

        uint vsm_index = 0;
        InterlockedAdd(vsm_info[0].visible_vsm_count, 16, vsm_index);

        for (uint i = 0; i < 16; ++i)
        {
            visible_vsm_list[vsm_index + i] = vsm_id + i;
        }

        InterlockedAdd(clear_virtual_page_table_indirect_args[0].z, 16);
    }
}