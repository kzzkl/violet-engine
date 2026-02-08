#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_vsm_ids;
    uint vsm_virtual_page_table;
    uint vsm_buffer;
    uint vsm_bounds_buffer;
};
PushConstant(constant_data, constant);

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];
    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];

    uint vsm_id = vsm_ids[dtid.z];

    uint virtual_page_index = get_virtual_page_index(vsm_id, dtid.xy);
    virtual_page_table[virtual_page_index] = 0;

    if (dtid.x == 0 && dtid.y == 0)
    {
        RWStructuredBuffer<vsm_bounds> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];
        vsm_bounds[vsm_id].required_bounds = uint4(VIRTUAL_PAGE_TABLE_SIZE, VIRTUAL_PAGE_TABLE_SIZE, 0, 0);
        vsm_bounds[vsm_id].invalidated_bounds = uint4(VIRTUAL_PAGE_TABLE_SIZE, VIRTUAL_PAGE_TABLE_SIZE, 0, 0);
    }
}