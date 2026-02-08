#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_vsm_ids;
    uint vsm_bounds_buffer;
    uint vsm_virtual_page_table;
};
PushConstant(constant_data, constant);

groupshared uint4 gs_required_bounds;
groupshared uint4 gs_invalidated_bounds;

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    if (group_index == 0)
    {
        gs_required_bounds.xy = VIRTUAL_PAGE_TABLE_SIZE;
        gs_required_bounds.zw = 0;
        gs_invalidated_bounds.xy = VIRTUAL_PAGE_TABLE_SIZE;
        gs_invalidated_bounds.zw = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    StructuredBuffer<uint> vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    
    uint2 virtual_page_coord = dtid.xy;
    uint vsm_id = vsm_ids[dtid.z];

    uint virtual_page_index = get_virtual_page_index(vsm_id, virtual_page_coord);
    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) != 0)
    {
        InterlockedMin(gs_required_bounds.x, virtual_page_coord.x);
        InterlockedMin(gs_required_bounds.y, virtual_page_coord.y);
        InterlockedMax(gs_required_bounds.z, virtual_page_coord.x);
        InterlockedMax(gs_required_bounds.w, virtual_page_coord.y);

        if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID) == 0)
        {
            InterlockedMin(gs_invalidated_bounds.x, virtual_page_coord.x);
            InterlockedMin(gs_invalidated_bounds.y, virtual_page_coord.y);
            InterlockedMax(gs_invalidated_bounds.z, virtual_page_coord.x);
            InterlockedMax(gs_invalidated_bounds.w, virtual_page_coord.y);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        RWStructuredBuffer<vsm_bounds> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];
        InterlockedMin(vsm_bounds[vsm_id].required_bounds.x, gs_required_bounds.x);
        InterlockedMin(vsm_bounds[vsm_id].required_bounds.y, gs_required_bounds.y);
        InterlockedMax(vsm_bounds[vsm_id].required_bounds.z, gs_required_bounds.z);
        InterlockedMax(vsm_bounds[vsm_id].required_bounds.w, gs_required_bounds.w);
        InterlockedMin(vsm_bounds[vsm_id].invalidated_bounds.x, gs_invalidated_bounds.x);
        InterlockedMin(vsm_bounds[vsm_id].invalidated_bounds.y, gs_invalidated_bounds.y);
        InterlockedMax(vsm_bounds[vsm_id].invalidated_bounds.z, gs_invalidated_bounds.z);
        InterlockedMax(vsm_bounds[vsm_id].invalidated_bounds.w, gs_invalidated_bounds.w);
    }
}