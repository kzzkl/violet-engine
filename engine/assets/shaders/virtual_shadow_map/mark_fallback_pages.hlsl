#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint vsm_info;
    uint visible_vsm_list;
    uint vsm_buffer;
    uint vsm_virtual_page_table;
    uint fallback_start;
};
PushConstant(constant_data, constant);

ConstantBuffer<camera_data> camera : register(b0, space1);

[shader("compute")]
[numthreads(64, 1, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
    StructuredBuffer<vsm_info> vsm_info = ResourceDescriptorHeap[constant.vsm_info];
    if (dtid.x >= vsm_info[0].visible_vsm_count)
    {
        return;
    }

    StructuredBuffer<uint> visible_vsm_list = ResourceDescriptorHeap[constant.visible_vsm_list];
    uint vsm_id = visible_vsm_list[dtid.x];

    StructuredBuffer<vsm_data> vsms = ResourceDescriptorHeap[constant.vsm_buffer];
    vsm_data vsm = vsms[vsm_id];

    if (vsm.cascade_index < constant.fallback_start)
    {
        return;
    }

    float4 position_ls = mul(vsm.matrix_vp, float4(camera.position, 1.0));
    position_ls /= position_ls.w;

    float2 virtual_page_coord = (position_ls.xy * 0.5 + 0.5) * VIRTUAL_PAGE_TABLE_SIZE;

    uint2 virtual_page_coord_min = floor(virtual_page_coord);
    uint2 virtual_page_coord_max = ceil(virtual_page_coord);

    RWStructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    uint4 virtual_page_indices = uint4(
        get_virtual_page_index(vsm_id, virtual_page_coord_min),
        get_virtual_page_index(vsm_id, uint2(virtual_page_coord_max.x, virtual_page_coord_min.y)),
        get_virtual_page_index(vsm_id, uint2(virtual_page_coord_min.x, virtual_page_coord_max.y)),
        get_virtual_page_index(vsm_id, virtual_page_coord_max));

    virtual_page_table[virtual_page_indices.x] |= VIRTUAL_PAGE_FLAG_VISIBLE;
    virtual_page_table[virtual_page_indices.y] |= VIRTUAL_PAGE_FLAG_VISIBLE;
    virtual_page_table[virtual_page_indices.z] |= VIRTUAL_PAGE_FLAG_VISIBLE;
    virtual_page_table[virtual_page_indices.w] |= VIRTUAL_PAGE_FLAG_VISIBLE;
}