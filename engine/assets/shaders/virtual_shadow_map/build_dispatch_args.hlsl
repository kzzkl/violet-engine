#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_info;
    uint visible_vsm_list;
    uint visible_virtual_page_list;
    uint visible_virtual_pages_indirect_args;
    uint visible_virtual_page_texels_indirect_args;
    uint vsm_virtual_page_table;
};
PushConstant(constant_data, constant);

groupshared uint gs_visible_virtual_page[64];
groupshared uint gs_visible_virtual_page_count;
groupshared uint gs_visible_virtual_page_offset;

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    if (group_index == 0)
    {
        gs_visible_virtual_page_count = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    StructuredBuffer<uint> visible_vsm_list = ResourceDescriptorHeap[constant.visible_vsm_list];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];

    uint vsm_id = visible_vsm_list[dtid.z];

    uint virtual_page_index = get_virtual_page_index(vsm_id, dtid.xy);
    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    if (virtual_page.flags & VIRTUAL_PAGE_FLAG_VISIBLE)
    {
        uint index;
        InterlockedAdd(gs_visible_virtual_page_count, 1, index);
        gs_visible_virtual_page[index] = virtual_page_index;
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        RWStructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
        InterlockedAdd(vsm_info[0].visible_virtual_page_count, gs_visible_virtual_page_count, gs_visible_virtual_page_offset);

        RWStructuredBuffer<dispatch_command> visible_virtual_pages_indirect_args = ResourceDescriptorHeap[constant.visible_virtual_pages_indirect_args];

        uint start = gs_visible_virtual_page_offset;
        uint end = gs_visible_virtual_page_offset + gs_visible_virtual_page_count;
        uint dispatch_count = (end + 63) / 64 - (start + 63) / 64;
        if (dispatch_count > 0)
        {
            InterlockedAdd(visible_virtual_pages_indirect_args[0].x, dispatch_count);
        }

        RWStructuredBuffer<dispatch_command> visible_virtual_page_texels_indirect_args = ResourceDescriptorHeap[constant.visible_virtual_page_texels_indirect_args];
        InterlockedAdd(visible_virtual_page_texels_indirect_args[0].z, gs_visible_virtual_page_count);
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index < gs_visible_virtual_page_count)
    {
        RWStructuredBuffer<uint> visible_virtual_page_list = ResourceDescriptorHeap[constant.visible_virtual_page_list];
        visible_virtual_page_list[gs_visible_virtual_page_offset + group_index] = gs_visible_virtual_page[group_index];
    }
}