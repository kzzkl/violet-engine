#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_vsm_ids;
    uint vsm_bounds_buffer;
    uint vsm_virtual_page_table;
};
PushConstant(constant_data, constant);

groupshared uint4 gs_view_aabb;

[numthreads(8, 8, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
    if (group_index == 0)
    {
        gs_view_aabb.xy = VIRTUAL_PAGE_TABLE_SIZE;
        gs_view_aabb.zw = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    StructuredBuffer<uint> vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];
    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    
    uint2 page_coord = dtid.xy;
    uint vsm_id = vsm_ids[dtid.z];

    uint virtual_page_index = get_virtual_page_index(vsm_id, page_coord);
    vsm_virtual_page virtual_page = unpack_virtual_page(virtual_page_table[virtual_page_index]);

    if ((virtual_page.flags & VIRTUAL_PAGE_FLAG_REQUEST) != 0 && (virtual_page.flags & VIRTUAL_PAGE_FLAG_CACHE_VALID) == 0)
    {
        InterlockedMin(gs_view_aabb.x, page_coord.x);
        InterlockedMin(gs_view_aabb.y, page_coord.y);
        InterlockedMax(gs_view_aabb.z, page_coord.x);
        InterlockedMax(gs_view_aabb.w, page_coord.y);
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        RWStructuredBuffer<uint4> vsm_bounds = ResourceDescriptorHeap[constant.vsm_bounds_buffer];
        InterlockedMin(vsm_bounds[vsm_id].x, gs_view_aabb.x);
        InterlockedMin(vsm_bounds[vsm_id].y, gs_view_aabb.y);
        InterlockedMax(vsm_bounds[vsm_id].z, gs_view_aabb.z);
        InterlockedMax(vsm_bounds[vsm_id].w, gs_view_aabb.w);
    }
}