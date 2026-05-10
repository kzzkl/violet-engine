#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_info;
    uint vsm_virtual_page_table;
    uint visible_virtual_page_list;
    uint render_virtual_page_list;
    uint render_virtual_page_indirect_args;
    uint max_render_pages_per_frame;
};
PushConstant(constant_data, constant);

groupshared uint gs_render_page_count;
groupshared uint gs_render_page_offset;

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    RWStructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    StructuredBuffer<uint> visible_virtual_page_list = ResourceDescriptorHeap[constant.visible_virtual_page_list];

    bool valid = dtid.x < vsm_info[0].visible_virtual_page_count;

    if (group_index == 0)
    {
        gs_render_page_count = 0;
        gs_render_page_offset = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint virtual_page_index = 0;
    vsm_virtual_page virtual_page = (vsm_virtual_page)0;
    bool render_page = false;

    if (valid)
    {
        virtual_page_index = visible_virtual_page_list[dtid.x];
        virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);
        render_page = !virtual_page.resident();
    }

    uint offset = 0;
    if (render_page)
    {
        InterlockedAdd(gs_render_page_count, 1, offset);
    }
    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0 && gs_render_page_count > 0)
    {
        InterlockedAdd(vsm_info[0].render_virtual_page_count, gs_render_page_count, gs_render_page_offset);
    }
    GroupMemoryBarrierWithGroupSync();

    if (render_page && (gs_render_page_offset + offset < constant.max_render_pages_per_frame))
    {
        virtual_page.flags |= VIRTUAL_PAGE_FLAG_RENDERING;
        virtual_page_table[virtual_page_index] = virtual_page.pack();
    }
}