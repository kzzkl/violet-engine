#include "common.hlsli"
#include "virtual_shadow_map/vsm_common.hlsli"

struct constant_data
{
    uint visible_virtual_page_list;
    uint render_physical_page_list;
    uint vsm_virtual_page_table;
    uint vsm_physical_shadow_map;
};
PushConstant(constant_data, constant);

[shader("compute")]
[numthreads(16, 16, 1)]
void cs_main(uint3 dtid : SV_DispatchThreadID)
{
#ifdef CLEAR_DYNAMIC
    StructuredBuffer<uint> visible_virtual_page_list = ResourceDescriptorHeap[constant.visible_virtual_page_list];
    uint virtual_page_index = visible_virtual_page_list[dtid.z];

    StructuredBuffer<uint> virtual_page_table = ResourceDescriptorHeap[constant.vsm_virtual_page_table];
    vsm_virtual_page virtual_page = vsm_virtual_page::unpack(virtual_page_table[virtual_page_index]);

    uint physical_page_index = get_physical_page_index(virtual_page.physical_page_coord);
#else
    StructuredBuffer<uint> render_physical_page_list = ResourceDescriptorHeap[constant.render_physical_page_list];
    uint physical_page_index = render_physical_page_list[dtid.z];
#endif

    uint2 physical_texel = get_physical_page_coord(physical_page_index) * PAGE_RESOLUTION + dtid.xy;

    RWTexture2D<uint> physical_shadow_map = ResourceDescriptorHeap[constant.vsm_physical_shadow_map];
    physical_shadow_map[physical_texel] = 0;
}