#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_light_count;
    uint visible_vsm_ids;
    uint vsm_buffer;
    uint vsm_projection_buffer;
    uint vsm_virtual_page_table;
};
PushConstant(constant_data, constant);

groupshared uint4 gs_view_aabb;

[numthreads(8, 8, 1)]
void calculate_view_aabb(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
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

    if (virtual_page.flags == VIRTUAL_PAGE_FLAG_REQUEST)
    {
        InterlockedMin(gs_view_aabb.x, page_coord.x);
        InterlockedMin(gs_view_aabb.y, page_coord.y);
        InterlockedMax(gs_view_aabb.z, page_coord.x);
        InterlockedMax(gs_view_aabb.w, page_coord.y);
    }

    GroupMemoryBarrierWithGroupSync();

    if (group_index == 0)
    {
        RWStructuredBuffer<vsm_projection> vsm_projections = ResourceDescriptorHeap[constant.vsm_projection_buffer];
        InterlockedMin(vsm_projections[vsm_id].aabb.x, gs_view_aabb.x);
        InterlockedMin(vsm_projections[vsm_id].aabb.y, gs_view_aabb.y);
        InterlockedMax(vsm_projections[vsm_id].aabb.z, gs_view_aabb.z);
        InterlockedMax(vsm_projections[vsm_id].aabb.w, gs_view_aabb.w);
    }
}

[numthreads(64, 1, 1)]
void update_projection(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<uint> visible_light_count = ResourceDescriptorHeap[constant.visible_light_count];
    if (dtid.x >= visible_light_count[1])
    {
        return;
    }

    StructuredBuffer<uint> vsm_ids = ResourceDescriptorHeap[constant.visible_vsm_ids];

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    RWStructuredBuffer<vsm_projection> vsm_projections = ResourceDescriptorHeap[constant.vsm_projection_buffer];

    uint vsm_id = vsm_ids[dtid.x];

    uint4 view_aabb = vsm_projections[vsm_id].aabb;
    if (view_aabb.x >= view_aabb.z || view_aabb.y >= view_aabb.w)
    {
        return;
    }

    int half_virtual_page_table_size = VIRTUAL_PAGE_TABLE_SIZE >> 1;
    float left = (int)view_aabb.x - half_virtual_page_table_size;
    float right = (int)view_aabb.z - half_virtual_page_table_size;
    float bottom = (int)view_aabb.y - half_virtual_page_table_size;
    float top = (int)view_aabb.w - half_virtual_page_table_size;

    float w = 1.0 / (right - left);
    float h = 1.0 / (top - bottom);

    float4x4 matrix_p = float4x4(
        w + w, 0, 0, (left + right) * -w,
        0, h + h, 0, (bottom + top) * -h,
        0, 0, 0, 0,
        0, 0, 0, 1);

    float4x4 matrix_p_full = vsms[vsm_id].matrix_p;
    matrix_p[2][2] = matrix_p_full[2][2]; // 1 / (far - near)
    matrix_p[2][3] = matrix_p_full[2][3]; // -near / (far - near)

    vsm_projections[vsm_id].matrix_p = matrix_p;
    vsm_projections[vsm_id].matrix_vp = matrix_p * vsms[vsm_id].matrix_v;
}