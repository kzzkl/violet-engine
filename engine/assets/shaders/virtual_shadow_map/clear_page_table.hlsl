#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_light_count;
    uint visible_vsm_ids;
    uint virtual_page_table;
    uint vsm_buffer;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> visible_light_count = ResourceDescriptorHeap[constant.visible_light_count];

    uint vsm_index = dtid.z;
    if (vsm_index >= visible_light_count[1])
    {
        return;
    }

    StructuredBuffer<uint> vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];
    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.virtual_page_table];

    uint virtual_page_index = get_virtual_page_index(vsm_ids[vsm_index], dtid.xy);
    virtual_page_table[virtual_page_index] = 0;
}