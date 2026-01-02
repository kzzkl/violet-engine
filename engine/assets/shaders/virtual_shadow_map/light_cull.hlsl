#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_light_count;
    uint visible_light_ids;
    uint visible_vsm_ids;
    uint vsm_dispatch_buffer;
    uint camera_id;
    uint directional_vsm_buffer;
};
PushConstant(constant_data, constant);

ConstantBuffer<scene_data> scene : register(b0, space1);
ConstantBuffer<camera_data> camera : register(b0, space2);

[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= scene.light_count)
    {
        return;
    }

    uint light_id = dtid.x;

    StructuredBuffer<light_data> lights = ResourceDescriptorHeap[scene.light_buffer];
    light_data light = lights[light_id];

    if (light.vsm_address == 0xFFFFFFFF)
    {
        return;
    }

    RWStructuredBuffer<uint> visible_light_count = ResourceDescriptorHeap[constant.visible_light_count];
    RWStructuredBuffer<uint> visible_light_ids = ResourceDescriptorHeap[constant.visible_light_ids];
    RWStructuredBuffer<uint> visible_vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];

    RWStructuredBuffer<dispatch_command> vsm_dispatch_commands = ResourceDescriptorHeap[constant.vsm_dispatch_buffer];

    if (light.type == LIGHT_DIRECTIONAL)
    {
        StructuredBuffer<uint> directional_vsms = ResourceDescriptorHeap[constant.directional_vsm_buffer];

        uint vsm_id = get_directional_vsm_id(directional_vsms, light.vsm_address, constant.camera_id);

        uint light_index = 0;
        InterlockedAdd(visible_light_count[0], 1, light_index);
        visible_light_ids[light_index] = dtid.x;

        uint vsm_index = 0;
        InterlockedAdd(visible_light_count[1], 16, vsm_index);

        for (uint i = 0; i < 16; ++i)
        {
            visible_vsm_ids[vsm_index + i] = vsm_id + i;
        }

        InterlockedAdd(vsm_dispatch_commands[0].z, 16);
    }
}