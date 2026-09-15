#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_vsm_list;
    uint virtual_page_table;
    uint vsm_buffer;
    uint vsm_bounds_buffer;
    uint render_coarse_page;
};
PushConstant(constant_data, constant);

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> visible_vsm_list = ResourceDescriptorHeap[constant.visible_vsm_list];
    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.virtual_page_table];

    uint vsm_id = visible_vsm_list[dtid.z];

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[vsm_id];

    uint virtual_page_index = get_virtual_page_index(vsm_id, dtid.xy);
    if (constant.render_coarse_page)
    {
        vsm_virtual_page virtual_page = (vsm_virtual_page)0;
        virtual_page.fallback_offset = vsm.cascade_index >= DIRECTIONAL_VSM_CASCADE_COUNT - DIRECTIONAL_VSM_CASCADE_COARSE_OFFSET ? 0 : DIRECTIONAL_VSM_CASCADE_COARSE_OFFSET;
        virtual_page_table[virtual_page_index] = virtual_page.pack();
    }
    else
    {
        virtual_page_table[virtual_page_index] = 0;
    }

    if (dtid.x == 0 && dtid.y == 0)
    {
        RWStructuredBuffer<vsm_bounds> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];
        vsm_bounds[vsm_id].required_bounds = uint4(VIRTUAL_PAGE_TABLE_SIZE, VIRTUAL_PAGE_TABLE_SIZE, 0, 0);
        vsm_bounds[vsm_id].invalidated_bounds = uint4(VIRTUAL_PAGE_TABLE_SIZE, VIRTUAL_PAGE_TABLE_SIZE, 0, 0);
    }
}