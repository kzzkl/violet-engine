#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_info;
    uint visible_vsm_list;
    uint visible_virtual_page_list;
    uint visible_virtual_page_indirect_args;
    uint visible_virtual_page_texels_indirect_args;
    uint vsm_buffer;
    uint virtual_page_table;
};
PushConstant(constant_data, constant);

groupshared uint gs_append_visible_virtual_page[64];
groupshared uint gs_append_visible_virtual_page_count;
groupshared uint gs_append_visible_virtual_page_offset;

[shader("compute")]
[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    if (group_index == 0)
    {
        gs_append_visible_virtual_page_count = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    StructuredBuffer<uint> visible_vsm_list = ResourceDescriptorHeap[constant.visible_vsm_list];
    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.virtual_page_table];

    uint vsm_id = visible_vsm_list[dtid.z];

    uint virtual_page_index = get_virtual_page_index(vsm_id, dtid.xy);

    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[vsm_id];

    uint fallback_page_index = 0xFFFFFFFF;
    if (virtual_page.visible() && !virtual_page.valid())
    {
        int2 world_page_coord = vsm.page_coord + dtid.xy;
        uint max_fallback_offset = min(DIRECTIONAL_VSM_CASCADE_COUNT - vsm.cascade_index, DIRECTIONAL_VSM_CASCADE_COARSE_OFFSET);

        for (uint i = 1; i < max_fallback_offset; ++i)
        {
            uint fallback_vsm_id = vsm_id + i;
            vsm_data fallback_vsm = vsms[fallback_vsm_id];

            int2 fallback_page_coord = 32 - fallback_vsm.page_coord + ((world_page_coord - 32) >> i);

            uint fallback_virtual_page_index = get_virtual_page_index(fallback_vsm_id, fallback_page_coord);
            vsm_virtual_page fallback_virtual_page = vsm_virtual_page::unpack(virtual_page_table[fallback_virtual_page_index]);

            if (fallback_virtual_page.valid())
            {
                virtual_page.fallback_offset = i;
                fallback_page_index = fallback_virtual_page_index;
                virtual_page_table[virtual_page_index] = virtual_page.pack();
                break;
            }
        }
    }

    if (fallback_page_index != 0xFFFFFFFF)
    {
        uint fallback_page;
        InterlockedOr(virtual_page_table[fallback_page_index], VIRTUAL_PAGE_FLAG_VISIBLE, fallback_page);

        if ((fallback_page & VIRTUAL_PAGE_FLAG_VISIBLE) == 0)
        {
            uint index;
            InterlockedAdd(gs_append_visible_virtual_page_count, 1, index);
            gs_append_visible_virtual_page[index] = fallback_page_index;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        RWStructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
        InterlockedAdd(vsm_info[0].visible_virtual_page_count, gs_append_visible_virtual_page_count, gs_append_visible_virtual_page_offset);

        RWStructuredBuffer<dispatch_command> visible_virtual_page_indirect_args = ResourceDescriptorHeap[constant.visible_virtual_page_indirect_args];

        uint dispatch_group_count = get_dispatch_group_count(gs_append_visible_virtual_page_offset, gs_append_visible_virtual_page_count, 64);
        if (dispatch_group_count > 0)
        {
            InterlockedAdd(visible_virtual_page_indirect_args[0].x, dispatch_group_count);
        }

        RWStructuredBuffer<dispatch_command> visible_virtual_page_texels_indirect_args = ResourceDescriptorHeap[constant.visible_virtual_page_texels_indirect_args];
        InterlockedAdd(visible_virtual_page_texels_indirect_args[0].z, gs_append_visible_virtual_page_count);
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index < gs_append_visible_virtual_page_count)
    {
        RWStructuredBuffer<uint> visible_virtual_page_list = ResourceDescriptorHeap[constant.visible_virtual_page_list];
        visible_virtual_page_list[gs_append_visible_virtual_page_offset + group_index] = gs_append_visible_virtual_page[group_index];
    }
}